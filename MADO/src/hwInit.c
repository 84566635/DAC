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
#include STMINCS(_tim)
#include <led.h>
#include <hwCommunicator.h>
#include <cc85xx_common.h>
#include <cc85xx_control_mst.h>
#include <hwCommunicatorSPIhelper.h>
#include <hwCommunicatorUARThelper.h>
#include <messageProcessorMADO.h>
#include "hwProfile.h"
#include <bqueue.h>
#include "audioTypes.h"
#include <convert.h>
#include <watchdog.h>
#include <cfg.h>
#include <i2sStream.h>

#include "wm8805.h"

#include <isrHandlersPrototypes.h>

static void I2S_Input_GPIO_Init(void);

void radio_int1_irq(void);
void radio_int2_irq(void);

///////////////////////////////////////////////////////////////////////////////////////
//Wypelnienie struktur wykorzystywanych przez moduly
///////////////////////////////////////////////////////////////////////////////////////

//Inicjalizacja helperow dla portow (CEN, PER1, PER2) zawierajace potrzebne dane zalezne od typu intefejsu.
// Mozliwe sa nastepujace konfiguracje (kazdy interfejs SPI powinien miec osobny modul (.moduleNum)):
// 1) UART - port jednoznacznie powiazany z interfejsem UART
// 2) SPI (radio) - powiazanie portu z interfejsem SPI przy czym mozliwe jest powiazanie kilku portow:
//    a) z jednym modulem radiowym. Wowczas porty sa rozrozniane po adresie hardwarowym (devAddress)
//    b) z jednym interfejsem SPI ale roznymi modulami radiowymi. Wowczas porty roznia sie pinem CS (i ew. adresem HW)
//    c) z osobnym interfejsem SPI.
//
// Dla UARTa potrzebne sa pola:
//   usart - adres peryferia uart/usart (USART1 ...)
// Dla SPI potrzebne sa pola:
//   moduleNum - index modulu obslugujcego konkretny interfejs SPI. Mozliwe jest przydzielenie kilku interfejsow do jednego modulu jednak
//               nie beda one wtedy obslugiwane rownolegle. Sugerowane jest uzywane osobnych moduluw (HWCOMM_SPI_NUM)
//   spi            -  adres peryferia spi (SPI1, ...)
//   cs {gpio, pin} - port i pin GPIO dla sygnalu CS
//   devAddress -   adres sprzetowy portu (np. MAC adres lub inny) uzywany do identyfikacji na radiu
//
static usartHelper_t CEN_helper __attribute__((section(".noload")));
# define CEN_PORT_DATA  PORT_DATA(USART, NULL, &CEN_helper, 0, 0, 1)

static spiHelper_t PER1_SPIhelper __attribute__((section(".noload")));
static spiHelper_t PER2_SPIhelper __attribute__((section(".noload")));
static spiHelper_t PER3_SPIhelper __attribute__((section(".noload")));
static spiHelper_t PER4_SPIhelper __attribute__((section(".noload")));
static spiHelper_t PER5_SPIhelper __attribute__((section(".noload")));
static spiHelper_t PER6_SPIhelper __attribute__((section(".noload")));
static spiHelper_t PER7_SPIhelper __attribute__((section(".noload")));
static spiHelper_t PER8_SPIhelper __attribute__((section(".noload")));
static usartHelper_t PER1_UARThelper __attribute__((section(".noload")));
static usartHelper_t PER2_UARThelper __attribute__((section(".noload")));
# define PER1_PORT_DATA  PORT_DATA(USART, &PER1_SPIhelper, &PER1_UARThelper,  0, 1, 1)
# define PER2_PORT_DATA  PORT_DATA(USART, &PER2_SPIhelper, &PER2_UARThelper,  0, 2, 1)
# define PER3_PORT_DATA  PORT_DATA(NONE,  &PER3_SPIhelper, NULL,              0, 1, 0)
# define PER4_PORT_DATA  PORT_DATA(NONE,  &PER4_SPIhelper, NULL,              0, 2, 0)
# define PER5_PORT_DATA  PORT_DATA(NONE,  &PER5_SPIhelper, NULL,              0, 1, 0)
# define PER6_PORT_DATA  PORT_DATA(NONE,  &PER6_SPIhelper, NULL,              0, 2, 0)
# define PER7_PORT_DATA  PORT_DATA(NONE,  &PER7_SPIhelper, NULL,              0, 1, 0)
# define PER8_PORT_DATA  PORT_DATA(NONE,  &PER8_SPIhelper, NULL,              0, 2, 0)
portData_t portData[PORTS_NUM] =
{
  [CEN_PORT] = CEN_PORT_DATA,
  [PER1_PORT] = PER1_PORT_DATA,
  [PER2_PORT] = PER2_PORT_DATA,
  [PER3_PORT] = PER3_PORT_DATA,
  [PER4_PORT] = PER4_PORT_DATA,
  [PER5_PORT] = PER5_PORT_DATA,
  [PER6_PORT] = PER6_PORT_DATA,
  [PER7_PORT] = PER7_PORT_DATA,
  [PER8_PORT] = PER8_PORT_DATA,
};
const char *portStr[PORTS_NUM] =
{
  [CEN_PORT]  = "CEN",
  [PER1_PORT] = "PER1",
  [PER2_PORT] = "PER2",
  [PER3_PORT] = "PER3",
  [PER4_PORT] = "PER4",
  [PER5_PORT] = "PER5",
  [PER6_PORT] = "PER6",
  [PER7_PORT] = "PER7",
  [PER8_PORT] = "PER8",
};

//Definicja LED (polaczenie z GPIO)
led_t leds[LEDS_NUM] =
{
  {GPIOB, GPIO_Pin_9,  0},  //LD1
  {GPIOD, GPIO_Pin_11, 0},  //LD2
  {GPIOD, GPIO_Pin_12, 0},  //LD3
  {GPIOD, GPIO_Pin_13, 0},  //LD4
  {GPIOD, GPIO_Pin_14, 0},  //LD5
  {GPIOD, GPIO_Pin_15, 0},  //LD6
};

static led_t ledsProto[LEDS_NUM] =
{
  {NULL, 0, 0},  //LD1
  {NULL, 0, 0},  //LD2
  {GPIOG, GPIO_Pin_13, 0},  //LD3
  {GPIOG, GPIO_Pin_14, 0},  //LD4
  {GPIOB, GPIO_Pin_13, 0},  //LD5
  {GPIOC, GPIO_Pin_5, 0},  //LD6
};

const constDMAsettings dmaStreamconstSettings[] =
{
  //  [BLOCK_A] = {
  //    SAI_BlockA_DMA2_StreamX, 0, 0, 0,
  //    DMA_IT_TCIF_BlockA, DMA_IT_TEIF_BlockA, DMA_IT_FEIF_BlockA, DMA_IT_DMEIF_BlockA,
  //    DMA_FLAG_TCIF_BlockA, DMA_FLAG_TEIF_BlockA, DMA_FLAG_FEIF_BlockA, DMA_FLAG_DMEIF_BlockA,
  //  },
  //  [BLOCK_B] = {
  //    SAI_BlockB_DMA2_StreamX, 0, 0, 0,
  //    DMA_IT_TCIF_BlockB, DMA_IT_TEIF_BlockB, DMA_IT_FEIF_BlockB, DMA_IT_DMEIF_BlockB,
  //    DMA_FLAG_TCIF_BlockB, DMA_FLAG_TEIF_BlockB, DMA_FLAG_FEIF_BlockB, DMA_FLAG_DMEIF_BlockB,
  //  },
  [I2S3_DMA] = {
    DMA1_Stream2, DMA_Channel_0, RCC_AHB1Periph_DMA1, DMA1_Stream2_IRQn,
    DMA_IT_TCIF2, DMA_IT_TEIF2, DMA_IT_FEIF2, DMA_IT_DMEIF2,
    DMA_FLAG_TCIF2, DMA_FLAG_TEIF2, DMA_FLAG_FEIF2, DMA_FLAG_DMEIF2,
  },
};


//Definicja interfejsow SPI (i polaczenie ich z DMA)
spiData_t spiData[] =
{
  [0] = {SPI1, DMA2_Stream3, DMA2_Stream2, DMA_Channel_3, DMA_Channel_3, DMA_IT_TCIF2},
};

//Funckja mapujaca SPI (SPI1,SPI2,SPI3) na index w tabeli spiData[]
int spiToSpiNum(void *spi)
{
  int spiNum = -1;
  switch ((uint32_t)spi)
    {
    case (uint32_t)SPI1:
      spiNum = 0;
      break;
    }
  massert(spiNum >= 0);
  return spiNum;
}

///////////////////////////////////////////////////////////////////////////////////////
//Definicje handlerow przerwan
///////////////////////////////////////////////////////////////////////////////////////

//Utworz handlery przerwan dla SPI i UART
#ifndef CEN_ALTER
USART_HANDLER(CEN_PORT, USART3);
#else
USART_HANDLER(CEN_PORT, USART1);
#endif
USART_HANDLER(PER1_PORT, USART2);
USART_HANDLER(PER2_PORT, UART4);

SPI_HANDLER(SPI1, 0);

//////// function prototypes:
static void initWM88xxInterface(spdifChip_t wmChip, uint32_t rxChannel, wm880x_audioMode_t wm880x_mode);
static void I2S_Input_GPIO_Init(void);
static void initWM88xxConfig(void);
static void ConfigureTimerForI2SClockMeasurement(void);
///////////////////////////////////////////////////////////////////////////////////////
//Inicjalizacja sprzetow specyficzna dla konfiguracji projektu (GPIO, DMA, SPI, UARTy)
///////////////////////////////////////////////////////////////////////////////////////
//USART_INIT(uart, apb, gpio, arg_tx, arg_rx, arg_speed)
//SPI_INIT(spi, apb, gpio, clk, miso, mosi, cs, dma)

void OTG_HS_IRQHandler(void);
int  libUSBStartup(int ulpi);


void hwBoardInit(void)
{
  //Load config into ram
  cfgInit();

  if(cfg.proto&0x1)
    memcpy((void*)leds, (void*)ledsProto, sizeof(leds));

  USART_INIT(UART8, 1, E, 1, E, 0, 921600); //DEBUG
  xprintf_init(sendChar);
  xprintf(CLEAR_SCREEN CUR_HOME(42) "\n");

  //MCLK key open
  PIN_SET(D, 5, 0);
  GPIO_INIT(D, 5, OUT, NOPULL);

  //Inicjalizacja LED
  LEDS_INIT();

  configureCKINpin();
  ConfigureTimerForI2SClockMeasurement();
  initWM88xxConfig();
#define CLR_HELPER(arg_name) memset((void *)&arg_name##helper, 0, sizeof(arg_name##helper))
#define CPY_HELPER(arg_dst, arg_src) memcpy((void *)&arg_dst##helper, (void *)&arg_src##helper, sizeof(arg_dst##helper));

  //inicjalizacja UARTow i SPI
  CLR_HELPER(CEN_);
  CLR_HELPER(PER1_SPI);
  CLR_HELPER(PER2_SPI);
  CLR_HELPER(PER3_SPI);
  CLR_HELPER(PER4_SPI);
  CLR_HELPER(PER5_SPI);
  CLR_HELPER(PER6_SPI);
  CLR_HELPER(PER7_SPI);
  CLR_HELPER(PER8_SPI);
  CLR_HELPER(PER1_UART);
  CLR_HELPER(PER2_UART);

#ifndef CEN_ALTER
  CEN_helper.usart = USART3;
  USART_INIT(USART3, 1, D, 8, D, 9, UART_SPEED); //CEN
  USART_INIT_HANDLER(USART3);
#else
  CEN_helper.usart = USART1;
  USART_INIT(USART1, 2, A, 9, A, 10, UART_SPEED); //CEN
  USART_INIT_HANDLER(USART1);
#endif

  PER1_UARThelper.usart = USART2;
  PER2_UARThelper.usart = UART4;

  if(cfg.proto & 0x40)
    {
      USART_INIT(USART2, 1, A, 2, D, 6, UART_SPEED); //PER1
    }
  else
    {
      USART_INIT(USART2, 1, A, 2, A, 3, UART_SPEED); //PER1
    }
  USART_INIT_HANDLER(USART2);
  USART_INIT(UART4, 1, A, 0, A, 1, UART_SPEED); //PER2
  USART_INIT_HANDLER(UART4);


  {
    //Radio modules
    SPI_INIT_HANDLER(SPI1);
    if(cfg.proto & 0x40)
      {
        SPI_INIT_MODULE(radioModule[0].spi,
                        SPI1, 2,
                        B, 3,
                        A, 6,
                        A, 7,
                        8, 32);
      }
    else
      {
        SPI_INIT_MODULE(radioModule[0].spi,
                        SPI1, 2,
                        A, 5,
                        A, 6,
                        A, 7,
                        8, 32);
      }
    radioModule[1] = radioModule[0];

    if(!(cfg.proto&0x1))
      {
        if(cfg.proto & 0x40)
          {
            SPI_CS_INIT_MODULE(radioModule[1].spi, G, 2);
          }
        else
          {
            SPI_CS_INIT_MODULE(radioModule[1].spi, A, 15);
          }

        SPI_CS_INIT_MODULE(radioModule[0].spi, C, 1);
      }
    else
      {
        SPI_CS_INIT_MODULE(radioModule[1].spi, C, 1);
        SPI_CS_INIT_MODULE(radioModule[0].spi, C, 2);
      }

    SETUP_EXTI(B, 4, EXTI4_IRQn, radio_int1_irq);

    if(cfg.proto & 0x40)
      {
        SETUP_EXTI(G, 1, EXTI1_IRQn, radio_int2_irq);
      }
    else
      {
        SETUP_EXTI(B, 5, EXTI9_5_IRQn, radio_int2_irq);
      }
  }

  {
    //PER1 section
    PER1_SPIhelper.moduleNum = 0;
    PER1_SPIhelper.master = 1;
    PER1_SPIhelper.radioModule = NULL;;
  }

  CPY_HELPER(PER2_SPI, PER1_SPI);
  CPY_HELPER(PER3_SPI, PER1_SPI);
  CPY_HELPER(PER4_SPI, PER1_SPI);
  CPY_HELPER(PER5_SPI, PER1_SPI);
  CPY_HELPER(PER6_SPI, PER1_SPI);
  CPY_HELPER(PER7_SPI, PER1_SPI);
  CPY_HELPER(PER8_SPI, PER1_SPI);

  portData[PER1_PORT].kond_pom = cfg.kondPomSPK[0];
  portData[PER2_PORT].kond_pom = cfg.kondPomSPK[0];
  portData[PER3_PORT].kond_pom = cfg.kondPomSPK[1];
  portData[PER4_PORT].kond_pom = cfg.kondPomSPK[1];
  portData[PER5_PORT].kond_pom = cfg.kondPomSPK[2];
  portData[PER6_PORT].kond_pom = cfg.kondPomSPK[2];
  portData[PER7_PORT].kond_pom = cfg.kondPomSPK[3];
  portData[PER8_PORT].kond_pom = cfg.kondPomSPK[3];

  {
    //  SAI BlockA configuration section:
    //    SAI1_SD_A    ->    PE6
    //    SAI1_FS_A    ->    PE4
    //    SAI1_MCLK_A  ->    PE2
    //    SAI1_SCK_A   ->    PE5
    SAI1_BLOCKx_GPIO_INIT(E, 2, 4, 5, 6);

    //  SAI BlockB configuration section:
    //    SAI1_SD_B   ->    PF6
    //    SAI1_MCLK_B ->    PF7
    //    SAI1_SCK_B  ->    PF8
    //    SAI1_FS_B   ->    PF9

    SAI1_BLOCKx_GPIO_INIT(F, 6, 7, 8, 9);

  }

  {
    if(cfg.cpha)
      {
        SPI_INIT_AUDIO_STREAM(SPI4, 2,
                              E, 11, 12, 13, 14,
                              Slave, SPI_CHUNK_SIZE, 2, SPI_CPHA_2Edge);
      }
    else
      {
        SPI_INIT_AUDIO_STREAM(SPI4, 2,
                              E, 11, 12, 13, 14,
                              Slave, SPI_CHUNK_SIZE, 2, SPI_CPHA_1Edge);
      }

    //Request data pin for spiAudioStream(BBB)
    PIN_SET(E, 9, 1);
    GPIO_INIT(E, 9, OUT, NOPULL);
    PIN_SET(E, 10, 1);
    GPIO_INIT(E, 10, OUT, NOPULL);

    DMA_INIT_SPI_AUDIO_STREAM(SPI4,
                              DMA2, 0, OLINUXINO_DMA_CHANNEL,
                              DMA_SPI_audioStream_ISR, SPI_CHUNK_SIZE);
  }

  //I2S input initialization
  I2S_Input_GPIO_Init();

  //SAIA Switch
  if(cfg.proto&0x40)
    {
      GPIO_INIT(G, 0, OUT, NOPULL);
    }
  else
    {
      GPIO_INIT(B, 0, OUT, NOPULL);
    }

  selectorSet(SELECTOR_SAI);

  if(cfg.proto&0x1)
    {
      //SAIB Switch
      GPIO_INIT(B, 1, OUT, NOPULL);
      PIN_SET(B, 1, 0);
    }

  //MCLK key close
  PIN_SET(D, 5, 1);


  //USB
  //ULPI reset
  PIN_SET(D, 2, 1);
  GPIO_INIT(D, 2, OUT, NOPULL);

  registerIRQ(OTG_HS_IRQn, OTG_HS_IRQHandler);
  NVIC_SetPriority(OTG_HS_IRQn, configMAX_SYSCALL_INTERRUPT_PRIORITY);
  libUSBStartup((cfg.proto & 0x40));
  NVIC_EnableIRQ(OTG_HS_IRQn);



  CONFIG_PRINT();
}

void selectorSet(int value)
{
  //0 - WM8804
  //1 - SAIA
  if(cfg.proto&0x40)
    {
      PIN_SET(G, 0, value==0);
    }
  else if(cfg.proto&0x1)
    {
      PIN_SET(B, 0, value!=0);
    }
  else
    {
      PIN_SET(B, 0, value==0);
    }
}

/**
  * @brief Initializes IOs used by the I2S input Audio (audio
  *        interfaces).
  * @param  None
  * @retval None
  */
static void I2S_Input_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  /* Enable I2S GPIO clocks */
  RCC_AHB1PeriphClockCmd( I2S_INPUT_GPIO_CLOCK , ENABLE);


  /* I2S_INPUT pins configuration: WS, SCK and SD pins -----------------------------*/
  GPIO_InitStructure.GPIO_Pin = I2S_INPUT_SCK_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(I2S_INPUT_SCK_GPIO, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = I2S_INPUT_SD_PIN;
  GPIO_Init(I2S_INPUT_SD_GPIO, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = I2S_INPUT_WS_PIN ;
  GPIO_Init(I2S_INPUT_WS_GPIO, &GPIO_InitStructure);

  /* Connect pins to I2S peripheral  */
  GPIO_PinAFConfig(I2S_INPUT_WS_GPIO, I2S_INPUT_WS_PINSRC, I2S_INPUT_GPIO_AF);
  GPIO_PinAFConfig(I2S_INPUT_SCK_GPIO, I2S_INPUT_SCK_PINSRC, I2S_INPUT_GPIO_AF);
  GPIO_PinAFConfig(I2S_INPUT_SD_GPIO, I2S_INPUT_SD_PINSRC, I2S_INPUT_GPIO_AF);


#ifdef CODEC_MCLK_ENABLED
  RCC_AHB1PeriphClockCmd( I2S_INPUT_MCK_GPIO, ENABLE);
  /* I2S_INPUT pins configuration: MCK pin */
  GPIO_InitStructure.GPIO_Pin = I2S_INPUT_MCK_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(I2S_INPUT_MCK_GPIO, &GPIO_InitStructure);
  /* Connect pins to I2S peripheral  */
  GPIO_PinAFConfig(I2S_INPUT_MCK_GPIO, I2S_INPUT_MCK_PINSRC, I2S_INPUT_GPIO_AF);
#endif /* CODEC_MCLK_ENABLED */
}


void configureCKINpin(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOC, ENABLE);
  /* I2S_INPUT pins configuration: MCK pin */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  /* Connect pins to I2S peripheral  */
#define GPIO_AF_I2S_CKIN ((uint8_t)0x05)
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource9, GPIO_AF_I2S_CKIN );

}

void ConfigureTimerForI2SClockMeasurement(void)
{
  TIM_TimeBaseInitTypeDef TIMInit;

  TIMInit.TIM_Prescaler = 168/2;
  TIMInit.TIM_Period = 32768*2-1;
  TIMInit.TIM_ClockDivision = TIM_CKD_DIV1;
  TIMInit.TIM_CounterMode = TIM_CounterMode_Up;

  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM14, ENABLE);
  TIM_TimeBaseInit(TIM14, &TIMInit);
  TIM_Cmd(TIM14, ENABLE);
}


const wm880x_t wm880x[WM_CHIP_CNT] =
{
  [WM8805_1] = { {GPIOG, GPIO_Pin_8} },
  [WM8805_2] = { {GPIOG, GPIO_Pin_9} },
  [WM8804_1] = { {GPIOG, GPIO_Pin_10} },
  [WM8804_2] = { {GPIOG, GPIO_Pin_11} }
};

static void initWM88xxInterface(spdifChip_t wmChip, uint32_t rxChannel, wm880x_audioMode_t wm880x_mode )
{
  char data[4];

  int timeout = 10000;
  while ( ( data[0] != 0x05 || data[1] != 0x88 ) && timeout-- )
    {
      WM88XXReadID(wmChip, data);
    }
  xprintf("ID1: %x ID2: %x REV: %x\n", data[0], data[1], data[2]);
  if (timeout <= 0)
    {
      xprintf("\e[31m""Chip %d not found\e[m\n", wmChip);
      return;
    }

  //Configure PLL
  WM88XXSetPLLMul(wmChip); //94.3104MHz/25MHz/2
  WM88XXWriteRegister(wmChip, WM8805_REG_PWRDN, PWRDN_SPDIFTXPD | PWRDN_SPDIFRXPD);
  WM88XXWriteRegister(wmChip, WM8805_REG_SPDMODE, SPDMODE_CMOS_INPUT);
  mdelay(10);

  switch (wmChip)
    {
    case WM8805_1:
    case WM8805_2:
      WM88XXSetReceiverChannel(wmChip, rxChannel);
      WM88XXWriteRegister(wmChip, WM8805_REG_PWRDN, PWRDN_SPDIFTXPD );
      xprintf("Enable SPDIF Receiver chip: %d\n", wmChip);
      WM88XXSetInterruptMask(wmChip, (char)(~IRQMASK_UPD_REC_FREQ));//Enable only UPD_REC_FREQ
      break;

    default:
      WM88XXSetTransmitterInput(wmChip, WM88XX_TX_INPUT_AIF);
      WM88XXWriteRegister(wmChip, WM8805_REG_PWRDN, PWRDN_SPDIFRXPD);
      xprintf("Enable SPDIF Transmitter chip: %d\n", wmChip);
      WM88XXSetInterruptMask(wmChip, 0xFF);//Disable all interrupts
      break;
    }

  xprintf("PLL set\n");

  switch (wmChip)
    {
    case WM8805_1:
    case WM8805_2:
      switch (wm880x_mode)
        {
        case WM880x_I2S:
          WM88XXSetAIFRXMode(wmChip, AIFRX_I2S_MODE | AIFRX_WL_24BITS_A | AIFRX_SYNC_OFF);
          WM88XXSetAIFTXMode(wmChip, AIFTX_I2S_MODE | AIFTX_WL_24BITS_A);
          break;

        case WM880x_DSP:
          WM88XXSetAIFRXMode(wmChip, AIFRX_DSP_MODE | AIFRX_WL_16BITS | AIFRX_LRP_INVERT | AIFRX_SYNC_OFF);
          WM88XXSetAIFTXMode(wmChip, AIFTX_DSP_MODE | AIFTX_WL_16BITS | AIFTX_LRP_INVERT);
          break;
        }
      //Enable master mode and enable MCLK pin as output
      WM88XXSetMode(wmChip, WM88XX_MASTER_MODE);
      //Continuous mode disable and enable 192kHz support
      WM88XXWriteRegister(wmChip, WM8805_REG_SPDRX1, SPDRX1_SPD_192K_EN);
      break;

    default:
      switch (wm880x_mode)
        {
        case WM880x_I2S:
          WM88XXSetAIFTXMode(wmChip, AIFTX_I2S_MODE | AIFTX_WL_16BITS );
          WM88XXSetAIFRXMode(wmChip, AIFRX_I2S_MODE | AIFRX_WL_16BITS );
          break;

        case WM880x_DSP:
          WM88XXSetAIFTXMode(wmChip, AIFTX_DSP_MODE | AIFTX_WL_16BITS | AIFTX_LRP_INVERT);
          WM88XXSetAIFRXMode(wmChip, AIFRX_DSP_MODE | AIFRX_WL_16BITS | AIFRX_LRP_INVERT);
          break;
        }
      //Enable slave mode and enable MCLK pin as input
      WM88XXSetMode(wmChip, WM88XX_SLAVE_MODE);
      //Continuous mode disable
      WM88XXWriteRegister(wmChip, WM8805_REG_SPDRX1, 0x00);
      break;
    }

  WM88XXIrqProc(wmChip);
  /*int cnt = 3;
  while (cnt--)
    {
      xprintf("irqX. 0x%x\n",  WM88XXReadRegister(wmChip, WM8805_REG_IRQSTAT));
      xprintf("spdif. 0x%x\n",  WM88XXReadRegister(wmChip, WM8805_REG_SPDIF));
      xprintf("rx1. 0x%x\n",  WM88XXReadRegister(wmChip, WM8805_REG_RXCH1));
      xprintf("rx2. 0x%x\n",  WM88XXReadRegister(wmChip, WM8805_REG_RXCH2));
      xprintf("rx3. 0x%x\n",  WM88XXReadRegister(wmChip, WM8805_REG_RXCH3));
      xprintf("rx4. 0x%x\n",  WM88XXReadRegister(wmChip, WM8805_REG_RXCH4));
      xprintf("rx5. 0x%x\n",  WM88XXReadRegister(wmChip, WM8805_REG_RXCH5));

      _my_delay(10000);
    }*/
}


static void initWM88xxConfig(void)
{
  //GPIO_INIT(B, 6, IN, UP);
  //GPIO_INIT(F, 0, IN, UP);
  //GPIO_INIT(F, 1, IN, UP);
  //GPIO_INIT(B, 7, IN, UP);

  //WM8805 mode select: SW/HW
  GPIO_INIT(G, 14, OUT, NOPULL);//B15
  SPI_CS_INIT(G, 8);
  //reset pin U4 U6
  GPIO_INIT(C, 14, OUT, NOPULL);
  //reset pin U1 U2
  GPIO_INIT(C, 13, OUT, NOPULL);

  RESET_WM5(RST_INACTIVE);

  //Disable others WM88XX
  GPIO_INIT(G, 8, OUT, NOPULL);//
  GPIO_INIT(G, 9, OUT, NOPULL);//
  GPIO_INIT(G, 10, OUT, NOPULL);//
  GPIO_INIT(G, 11, OUT, NOPULL);//
  PIN_SET(G, 8, 1);
  PIN_SET(G, 9, 1);
  PIN_SET(G, 10, 1);
  PIN_SET(G, 11, 1);


  PIN_SET(G, 14, WM_SW_MODE);

  mdelay(100);

  RESET_WM5(RST_ACTIVE);
  RESET_WM4(RST_ACTIVE);
  // PIN_SET(C, 13, 0);

  mdelay(100);

  RESET_WM5(RST_INACTIVE);
  RESET_WM4(RST_INACTIVE);

  mdelay(100);

  //WM1 and WM2 audio source select ports
  // SRC_SEL_RADIO1(RADIO1_SRC_WM8805_1);
  if(cfg.proto & 0x40)
    {
      GPIO_INIT(G, 0, OUT, NOPULL);
      PIN_SET(G, 0, RADIO1_SRC_SAI_A);
    }
  else
    {
      GPIO_INIT(B, 0, OUT, NOPULL);
      PIN_SET(B, 0, RADIO1_SRC_SAI_A);
    }

  // #define SPI_INIT(arg_spi, arg_apb,
  //                  arg_gpio, arg_clk, arg_miso, arg_mosi,
  //                  arg_dataSize, arg_prescaler  )
  SPI_INIT(SPI6, 2,
           G, 13, 12, 14,
           8, 256);

  SPI_Cmd (SPI6, ENABLE);


  WM88XXInit();
  // Detecting all chips:
  char data[4];
  int wm = 0, timeout = 0;
  for (wm = WM8805_1; wm < WM_CHIP_CNT; wm++)
    {
      timeout = 1000;
      while ( ( data[0] != 0x05 || data[1] != 0x88 )  && timeout )
        {
          WM88XXReadID(wm, data);
          timeout--;
        }
      xprintf("ID1: %x ID2: %x REV: %x\n", data[0], data[1], data[2]);
      *((int *)data) = 0;

      if (timeout <= 0)
        {
          xprintf("\e[31m""Chip %d not found\e[m\n", wm);
        }
      else
        {
          xprintf("Identifed chip %d\n", wm);
        }
    }

  xprintf("Ver 0.000001\n");

  xprintf("Config for WM8805_1\n");
  initWM88xxInterface(WM8805_1, WM88XX_RECV_CHANNEL0, WM880x_I2S);    //default OPT1
  xprintf("Config for WM8805_2\n");
  initWM88xxInterface(WM8805_2, WM88XX_RECV_CHANNEL0, WM880x_DSP);  // DSP for direct feed to WM8804_1

  xprintf("Config for WM8804_1\n");
  initWM88xxInterface(WM8804_1, WM88XX_RECV_CHANNEL1, WM880x_DSP);
  xprintf("Config for WM8804_2\n");
  initWM88xxInterface(WM8804_2, WM88XX_RECV_CHANNEL1, WM880x_DSP);

  wm = WM8805_1;
  while (0)
    {
      xprintf("Status for WM %d\n", wm);
      xprintf("\tirqX. 0x%x\n",  WM88XXReadRegister(wm, WM8805_REG_IRQSTAT));
      xprintf("\tspdif. 0x%x\n",  WM88XXReadRegister(wm, WM8805_REG_SPDIF));
      xprintf("\trx1. 0x%x\n",  WM88XXReadRegister(wm, WM8805_REG_RXCH1));
      xprintf("\trx2. 0x%x\n",  WM88XXReadRegister(wm, WM8805_REG_RXCH2));
      xprintf("\trx3. 0x%x\n",  WM88XXReadRegister(wm, WM8805_REG_RXCH3));
      xprintf("\trx4. 0x%x\n",  WM88XXReadRegister(wm, WM8805_REG_RXCH4));
      xprintf("\trx5. 0x%x\n",  WM88XXReadRegister(wm, WM8805_REG_RXCH5));


      WM88XXIrqProc(wm);
      mdelay(1000);
      xprintf("TIM14CNT: %d\n", TIM14->CNT);
      //      TIM14->CNT = 0;
      wm++;

      wm = wm % 2;

    }
  mdelay(10);
}

static int freqs[2] = {44100,44100};
#define RETRY_TO_CONFIRM 5
static int freqsZero[2] = {RETRY_TO_CONFIRM,RETRY_TO_CONFIRM};
void wdogAssert(int value)
{
  if(value)
    {
      int freq = WM88XXIrqProc(WM8805_2);
      if(freqs[0] != freq)
        {
          if(freqsZero[0] > 0)
            {
              freqsZero[0]--;
            }
          else
            {
              switch(freq)
                {
                case 44100:
                case 48000:
                  formatChange(INPUT_SPDIF1, IF_16L16N16P16N, freq);
                  break;
                case 88200:
                case 96000:
                case 176400:
                case 192000:
                default:
                  formatChange(INPUT_SPDIF1, IF_16L16N16P16N, 0);
                  break;
                }
              freqs[0] = freq;
              freqsZero[0] = RETRY_TO_CONFIRM;
            }
        }

      freq = 0;
      if(WM88XXGetLock(WM8805_1)) freq = i2sGetFreq();
      if(!freq) freq  = WM88XXIrqProc(WM8805_1);
      if(freqs[1] != freq)
        {
          if(freqsZero[1] > 0)
            {
              freqsZero[1]--;
            }
          else
            {
              switch(freq)
                {
                case 44100:
                case 48000:
                  formatChange(INPUT_SPDIF2, IF_16L16N16P16N, freq);
                  break;
                case 88200:
                case 96000:
                case 176400:
                case 192000:
                  formatChange(INPUT_SPDIF2, IF_16L8L8N16P8P8N, freq);
                  break;
                default:
                  formatChange(INPUT_SPDIF2, IF_16L16N16P16N, 0);
                  break;
                }
              freqs[1] = freq;
              freqsZero[1] = RETRY_TO_CONFIRM;
            }
        }
    }
}

//radio interrupts
void radio_int1_irq(void)
{
  // void EXTI0_IRQHandler(void)
  if (EXTI_GetITStatus(EXTI_Line4) != RESET)
    {
      SPIevent(PORT_INVALID, SPI_EV_TYPE_DATA_READY);
      /* Clear the EXTI line 0 pending bit */
      EXTI_ClearITPendingBit(EXTI_Line4);
    }
}

void radio_int2_irq(void)
{
  uint32_t extiLine = EXTI_Line5;
  if(cfg.proto & 0x40)
    extiLine = EXTI_Line1;
    
    // void EXTI0_IRQHandler(void)
  if (EXTI_GetITStatus(extiLine) != RESET)
    {
      SPIevent(PORT_INVALID, SPI_EV_TYPE_DATA_READY);
      /* Clear the EXTI line 0 pending bit */
      EXTI_ClearITPendingBit(extiLine);
    }
}

CONFIG_CONST;

const portNum_t portCompl[PORTS_NUM] =
{
  CEN_PORT,
  PER2_PORT,
  PER1_PORT,
  PER4_PORT,
  PER3_PORT,
  PER6_PORT,
  PER5_PORT,
  PER8_PORT,
  PER7_PORT,
};

uint32_t HAL_GetTick(void);
uint32_t HAL_GetTick(void)
{
  return xTaskGetTickCount();
}
