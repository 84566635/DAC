#include <project.h>
#include <common.h>
#include <startup.h>
#include <string.h>
#include <FreeRTOS.h>
#include <semphr.h>
#include <misc.h>
#include STMINCP()
#include STMINCS(_rcc)
#include STMINCS(_gpio)
#include STMINCS(_usart)
#include STMINCS(_spi)
#include STMINCS(_dma)
#include STMINCS(_hwInit)
#include STMINCS(_exti)
#include STMINCS(_syscfg)
#include <led.h>
#include <hwCommunicator.h>
#include <cc85xx_common.h>
#include <cc85xx_control_mst.h>
#include <hwCommunicatorSPIhelper.h>
#include <hwCommunicatorUARThelper.h>
#include "hwProfile.h"
#include <bqueue.h>
#include "audioTypes.h"
#include <convert.h>
#include "wm8805.h"
#include <debug.h>

extern const wm880x_t wm880x[WM_CHIP_CNT];

static char WM_Register_PLL6[WM_CHIP_CNT]   = {0x18, 0x18, 0x18, 0x18};
static char WM_Register_AIFRX[WM_CHIP_CNT]  = {0x06, 0x06, 0x06, 0x06};
static char WM_Register_AIFTX[WM_CHIP_CNT]  = {0x06, 0x06, 0x06, 0x06};
static char WM_Register_SPDTX4[WM_CHIP_CNT] = {0x71, 0x71, 0x71, 0x71};

//const int FreqHz[] = {44100, 0, 48000, 32000, 22050, 0, 24000, 0, 88200, 0, 96000, 0, 176400, 0, 0, 192000};

static const int FreqHz[4][16] =
{
  {176400, 0, 192000, 0, 0, 0, 0, 0, 88200, 0, 96000, 0, 176400, 0, 0, 192000},
  {88200, 0, 96000, 0, 0, 0, 0, 0, 88200, 0, 96000, 0, 176400, 0, 0, 192000},
  {44100, 0, 48000, 0, 0, 0, 0, 0, 88200, 0, 96000, 0, 176400, 0, 0, 192000},
  {000, 0, 000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

static xSemaphoreHandle mutex;
void WM88XXInit(void)
{
  massert((mutex = xSemaphoreCreateMutex()));
}
int WM88XXWriteRegister(spdifChip_t spdifChip, int reg, int val)
{
  if( xSemaphoreTake( mutex, portMAX_DELAY) != pdTRUE )
    return 0;
  int tmp;

  ENABLE_WM(spdifChip);

  while (SPI_I2S_GetFlagStatus(SPI6, SPI_I2S_FLAG_BSY) == SET);

  SPI_I2S_SendData(SPI6, WM_WRITE | reg);

  while (SPI_I2S_GetFlagStatus(SPI6, SPI_I2S_FLAG_RXNE) != SET);
  tmp = SPI_I2S_ReceiveData(SPI6);

  SPI_I2S_SendData(SPI6, val);
  while (SPI_I2S_GetFlagStatus(SPI6, SPI_I2S_FLAG_RXNE) != SET);
  tmp = SPI_I2S_ReceiveData(SPI6);

  while (SPI_I2S_GetFlagStatus(SPI6, SPI_I2S_FLAG_BSY) == SET);

  DISABLE_WM(spdifChip);
  xSemaphoreGive(mutex);

  return tmp;
}

int WM88XXReadRegister(spdifChip_t spdifChip, int reg)
{
  if( xSemaphoreTake( mutex, portMAX_DELAY) != pdTRUE )
    return 0;
  int tmp;

  ENABLE_WM(spdifChip);
  while (SPI_I2S_GetFlagStatus(SPI6, SPI_I2S_FLAG_BSY) == SET);

  SPI_I2S_SendData(SPI6, WM_READ | reg);

  while (SPI_I2S_GetFlagStatus(SPI6, SPI_I2S_FLAG_RXNE) != SET);
  tmp = SPI_I2S_ReceiveData(SPI6);

  SPI_I2S_SendData(SPI6, 0xFF);
  while (SPI_I2S_GetFlagStatus(SPI6, SPI_I2S_FLAG_RXNE) != SET);
  tmp = SPI_I2S_ReceiveData(SPI6);

  while (SPI_I2S_GetFlagStatus(SPI6, SPI_I2S_FLAG_BSY) == SET);
  DISABLE_WM(spdifChip);

  xSemaphoreGive(mutex);
  return tmp;
}

void WM88XXSoftwareReset(spdifChip_t spdifChip)
{
  WM88XXWriteRegister(spdifChip, WM8805_REG_RSTDEVID1, 0x00);
}

void WM88XXReadID(spdifChip_t spdifChip, char *tab )
{
  *tab++ = WM88XXReadRegister(spdifChip, WM8805_REG_RSTDEVID1);
  *tab++ = WM88XXReadRegister(spdifChip, WM8805_REG_DEVID2);
  *tab   = WM88XXReadRegister(spdifChip, WM8805_REG_DEVREV);
}
static void WM88XXSetPLL(spdifChip_t spdifChip, int plln, int pllk)
{

  WM88XXWriteRegister(spdifChip, WM8805_REG_PLL1, pllk & 0xFF);
  WM88XXWriteRegister(spdifChip, WM8805_REG_PLL2, (pllk >> 8) & 0xFF);
  WM88XXWriteRegister(spdifChip, WM8805_REG_PLL3, (pllk >> 16) & 0x3F);
  //If crystal freq is higher than 24MHz set PRESCALE bit(divider before pll /2)
  WM88XXWriteRegister(spdifChip, WM8805_REG_PLL4, (plln & 0x0F) | PRESCALER*PLL4_PRESCALE);
  //  WM88XXWriteRegister(spdifChip, WM8805_REG_PLL5, (1<<2) | (1<<4) | (2<<0) );
}

void WM8805DBG(int *bufSize, char *bufPtr)
{
  spdifChip_t spdifChip;
  for(spdifChip = WM8805_1; spdifChip <= WM8805_2; spdifChip++)
    {
      DPRINTF("WM8805_%d %02x %02x %02x %02x %02x %02x %02x"CLEAR_LINE"\n", spdifChip,
              WM88XXReadRegister(spdifChip, WM8805_REG_SPDIF),
              WM88XXReadRegister(spdifChip, WM8805_REG_IRQSTAT),
              WM88XXReadRegister(spdifChip, WM8805_REG_RXCH1),
              WM88XXReadRegister(spdifChip, WM8805_REG_RXCH2),
              WM88XXReadRegister(spdifChip, WM8805_REG_RXCH3),
              WM88XXReadRegister(spdifChip, WM8805_REG_RXCH4),
              WM88XXReadRegister(spdifChip, WM8805_REG_RXCH5));
    }
}

void WM88XXSetPLLMul(spdifChip_t spdifChip)
{
  int lock = !((WM88XXReadRegister(spdifChip, WM8805_REG_SPDIF)>>6)&1);
  if(!lock)
    {
      //Set defaults
      WM88XXSetPLL(spdifChip, WM88XX_PLLN_94_3104MHz, WM88XX_PLLK_94_3104MHz);
      //Check lock
      int retry = 20;
      do
        {
          mdelay(1);
          lock = !((WM88XXReadRegister(spdifChip, WM8805_REG_SPDIF)>>6)&1);
        }
      while(retry-- && !lock);

      //Still no lock
      if(!lock)
        {
          //Check if 192kHz
          int recovered = (WM88XXReadRegister(spdifChip, WM8805_REG_SPDIF)>>4)&3;
          if(recovered == 0)
            {
              //Maybe 192 kHz
              WM88XXSetPLL(spdifChip, WM88XX_PLLN_98_304MHz, WM88XX_PLLK_98_304MHz);
            }
        }
    }
}
static const char *wmStr[4] =
{
  "WM8805_1",
  "WM8805_2",
  "WM8804_1",
  "WM8804_2",
};

void WM88XXSetReceiverChannel(spdifChip_t spdifChip, int channel)
{
  if (channel < 0 || channel > 7)
    channel = 0;

  WM_Register_PLL6[spdifChip] = (WM_Register_PLL6[spdifChip] & (~0x07)) | (char)channel;
  WM88XXWriteRegister(spdifChip, WM8805_REG_PLL6, WM_Register_PLL6[spdifChip]);

  dprintf(LL_DEBUG, "Set receiver channel: %d in %s\n", channel, wmStr[spdifChip]);
  WM88XXIrqProc(spdifChip);
}

void WM88XXSetTransmitterInput(spdifChip_t spdifChip, int input)
{
  WM_Register_SPDTX4[spdifChip] = (WM_Register_SPDTX4[spdifChip] & (~0x40)) | (char)input;
  WM88XXWriteRegister(spdifChip, WM8805_REG_SPDTX4, WM_Register_SPDTX4[spdifChip]);
}

void WM88XXSetAIFRXMode(spdifChip_t spdifChip, char val)
{
  WM_Register_AIFRX[spdifChip] = (WM_Register_AIFRX[spdifChip] & 0x00) | (val & 0xfF);
  WM88XXWriteRegister(spdifChip, WM8805_REG_AIFRX, WM_Register_AIFRX[spdifChip]);
}

void WM88XXSetAIFTXMode(spdifChip_t spdifChip, char val)
{
  WM_Register_AIFTX[spdifChip] = (WM_Register_AIFTX[spdifChip] & 0xC0) | (val & 0x3F);
  WM88XXWriteRegister(spdifChip, WM8805_REG_AIFTX, WM_Register_AIFTX[spdifChip]);
}

void WM88XXSetMode(spdifChip_t spdifChip, char val)
{
  if (val)
    WM_Register_AIFRX[spdifChip] |= WM88XX_MASTER_MODE;
  else
    WM_Register_AIFRX[spdifChip] &= (~WM88XX_MASTER_MODE);

  WM88XXWriteRegister(spdifChip, WM8805_REG_AIFRX, WM_Register_AIFRX[spdifChip]);
}

int WM88XXGetLock(spdifChip_t spdifChip)
{
  return !((WM88XXReadRegister(spdifChip, WM8805_REG_SPDIF)>>6)&1);
}

int WM88XXReturnCurrentFreq(spdifChip_t spdifChip)
{
  int lock = !((WM88XXReadRegister(spdifChip, WM8805_REG_SPDIF)>>6)&1);
  if(lock)
    {
      int recovered = (WM88XXReadRegister(spdifChip, WM8805_REG_SPDIF)>>4)&3;
      int readFreq = WM88XXReadRegister(spdifChip, WM8805_REG_RXCH4) & 0x0F;
      lock = !((WM88XXReadRegister(spdifChip, WM8805_REG_SPDIF)>>6)&1);
      return lock?FreqHz[recovered][readFreq]:0;
    }
  return 0;
}

void WM88XXSetInterruptMask(spdifChip_t spdifChip, char mask)
{
  WM88XXWriteRegister(spdifChip, WM8805_REG_IRQMASK, mask);
}

int WM88XXIrqProc(spdifChip_t spdifChip)
{
  WM88XXSetPLLMul(spdifChip);
  return WM88XXReturnCurrentFreq(spdifChip);
  ;
}

int WM88XX_AudioInterfaceSetState(spdifChip_t spdifChip, uint8_t state)
{
  if (state == ENABLE)
    WM88XXWriteRegister(spdifChip, WM8805_REG_PWRDN, PWRDN_SPDIFTXPD);
  else if (state == DISABLE)
    WM88XXWriteRegister(spdifChip, WM8805_REG_PWRDN, PWRDN_SPDIFTXPD | PWRDN_AIFPD);
  else
    massert(0);

  return 0;
}

void WM8804setResolution(spdifChip_t wmChip, int resolution)
{

  if(resolution == 16)
    {
      WM88XXSetAIFTXMode(wmChip, AIFTX_DSP_MODE | AIFTX_WL_16BITS | AIFTX_LRP_INVERT);
      WM88XXSetAIFRXMode(wmChip, AIFRX_DSP_MODE | AIFRX_WL_16BITS | AIFRX_LRP_INVERT);
    }
  else
    {
      WM88XXSetAIFTXMode(wmChip, AIFTX_DSP_MODE | AIFTX_WL_24BITS_A | AIFTX_LRP_INVERT);
      WM88XXSetAIFRXMode(wmChip, AIFRX_DSP_MODE | AIFRX_WL_24BITS_A | AIFRX_LRP_INVERT);
    }
}

