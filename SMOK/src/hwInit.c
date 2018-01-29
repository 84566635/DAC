#include <project.h>
#include <common.h>
#include <stdint.h>
#include <startup.h>
#include <string.h>
#include <FreeRTOS.h>
#include <semphr.h>
#include <misc.h>
#include STMINCP()
#include STMINCS(_rcc)
#include STMINCS(_gpio)
#include STMINCS(_exti)
#include STMINCS(_syscfg)
#include STMINCS(_usart)
#include STMINCS(_spi)
#include STMINCS(_dma)
#include STMINCS(_hwInit)
#include <led.h>
#include <hwCommunicator.h>
#include <hwCommunicatorSPIhelper.h>
#include <hwCommunicatorUARThelper.h>
#include <messageProcessorSMOK.h>
#include <watchdog.h>
#include <uit_monitor.h>
#include <i2c.h>
#include <math.h>
#include <cfg.h>
#include STMINCS(_iwdg)


void radioIntIRQ(void);

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
//   usart - adres peryferia uart/usart (USART1, ...)
// Dla SPI potrzebne sa pola:
//   moduleNum - index modulu obslugujcego konkretny interfejs SPI. Mozliwe jest przydzielenie kilku interfejsow do jednego modulu jednak
//               nie beda one wtedy obslugiwane rownolegle. Sugerowane jest uzywane osobnych moduluw (HWCOMM_SPI_NUM)
//   spi            -  adres peryferia spi (SPI1, ...)
//   cs {gpio, pin} - port i pin GPIO dla sygnalu CS
//   devAddress -   adres sprzetowy portu (np. MAC adres lub inny) uzywany do identyfikacji na radiu
//
static usartHelper_t CEN_UARThelper __attribute__((section(".noload")));
static spiHelper_t CEN_SPIhelper __attribute__((section(".noload")));
# define CEN_PORT_DATA  PORT_DATA(USART, &CEN_SPIhelper, &CEN_UARThelper, 0, 0, 1)
static usartHelper_t PER1_UARThelper __attribute__((section(".noload")));
static usartHelper_t PER2_UARThelper __attribute__((section(".noload")));
# define PER1_PORT_DATA  PORT_DATA(USART, NULL, &PER1_UARThelper, 0, 0, 1)
# define PER2_PORT_DATA  PORT_DATA(USART, NULL, &PER2_UARThelper, 0, 0, 1)
portData_t portData[PORTS_NUM] =
{
  [CEN_PORT] = CEN_PORT_DATA,
  [PER1_PORT] = PER1_PORT_DATA,
  [PER2_PORT] = PER2_PORT_DATA,
};
const char *portStr[PORTS_NUM] =
{
  [CEN_PORT]  = "CEN",
  [PER1_PORT] = "PER1",
  [PER2_PORT] = "PER2",
};

//Definicja LED (polaczenie z GPIO)
led_t leds[LEDS_NUM] =
{
  {GPIOC, GPIO_Pin_6},
  {GPIOC, GPIO_Pin_7},
  {GPIOC, GPIO_Pin_8},
  {GPIOC, GPIO_Pin_9},
  {GPIOC, GPIO_Pin_10},
};

//Definicja interfejsow SPI (i polaczenie ich z DMA)
spiData_t spiData[] =
{
  [0] = {SPI1, DMA2_Stream3, DMA2_Stream2, DMA_Channel_3, DMA_Channel_3, DMA_IT_TCIF2},
  [1] = {SPI2, DMA1_Stream4, DMA1_Stream3, DMA_Channel_0, DMA_Channel_0, DMA_IT_TCIF3},
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
    case (uint32_t)SPI2:
      spiNum = 1;
      break;
    }
  massert(spiNum >= 0);
  return spiNum;
}

///////////////////////////////////////////////////////////////////////////////////////
//Definicje handlerow przerwan
///////////////////////////////////////////////////////////////////////////////////////

//Utworz handlery przerwan dla SPI i UART
USART_HANDLER(CEN_PORT, UART4);
USART_HANDLER(PER1_PORT, USART2);
USART_HANDLER(PER2_PORT, USART3);

SPI_HANDLER(SPI1, 0);

//Sharc
SPI_DMA_HANDLER(DMA1, 3, 1);
static spi_t sharcSpi;

void sharcTransfer(uint8_t *txBuffer, uint8_t *rxBuffer, int size)
{

  //CS low
  GPIO_WriteBit(GPIOB, GPIO_Pin_12, 0);
  mdelay(1);
  configureDMAforSPI(sharcSpi.spi, txBuffer, rxBuffer, size);
  mdelay(1);

  //CS high
  GPIO_WriteBit(GPIOB, GPIO_Pin_12, 1);
}

void sharc_spi_init(void)
{
  //Sharck SPI
  int prescaler;
  switch(cfg.sharcSPISpeed)
    {
    case 7:
      prescaler = SPI_BaudRatePrescaler_2;
      break;
    case 6:
      prescaler = SPI_BaudRatePrescaler_4;
      break;
    case 5:
      prescaler = SPI_BaudRatePrescaler_8;
      break;
    case 4:
      prescaler = SPI_BaudRatePrescaler_16;
      break;
    case 3:
      prescaler = SPI_BaudRatePrescaler_32;
      break;
    case 2:
      prescaler = SPI_BaudRatePrescaler_64;
      break;
    case 1:
      prescaler = SPI_BaudRatePrescaler_128;
      break;
    default:
      prescaler = SPI_BaudRatePrescaler_256;
      break;
    }


  SPI_INIT_DMA_HANDLER(DMA1, 3);
  SPI_INIT_DMA_MODULE(sharcSpi,
                      SPI2, 1,
                      B, 13, 14, 15,
                      8, prescaler,
                      DMA1);
  SPI_CS_INIT_MODULE(sharcSpi, B, 12);


}
///////////////////////////////////////////////////////////////////////////////////////
//Inicjalizacja sprzetow specyficzna dla konfiguracji projektu (GPIO, DMA, SPI, UARTy)
///////////////////////////////////////////////////////////////////////////////////////
//USART_INIT(uart, apb, gpio, arg_tx, arg_rx, arg_speed)
//SPI_INIT(spi, apb, gpio, clk, miso, mosi, cs, dma)
void hwBoardInit(void)
{
  //SPI pullups
  GPIO_INIT(B, 12, IN, UP);
  GPIO_INIT(B, 13, IN, UP);
  GPIO_INIT(B, 14, IN, UP);
  GPIO_INIT(B, 15, IN, UP);


  //Load config into ram
  cfgInit();

  USART_INIT(UART5, 1, C, 12, D, 2, 921600); //DEBUG
  xprintf_init(sendChar);
  xprintf(CLEAR_SCREEN CUR_HOME(19) "\n");

  //Inicjalizacja LED
  LEDS_INIT_ON_GPIO(C);

  //POWER_ON
  GPIO_WriteBit(GPIOC, GPIO_Pin_11, 1);
  GPIO_INIT(C, 11, OUT, NOPULL);

  //RST AMP
  if(!cfg.no_rst_amp)
    {
      GPIO_WriteBit(GPIOA, GPIO_Pin_11, 0);
    }
  else
    {
      GPIO_WriteBit(GPIOA, GPIO_Pin_11, 1);
    }

  powerOFF1();
  powerOFF2();
  powerOFF3();

  GPIO_INIT(A, 11, OUT, NOPULL);
  //SPK SWITCH
  GPIO_INIT(A, 9, OUT, NOPULL);

  xprintf("Wait 1000ms to stabilize power before external devices configuration\n");
  mdelay(1000);

  //inicjalizacja UARTow i SPI
  memset((void *)&CEN_SPIhelper, 0, sizeof(CEN_SPIhelper));
  memset((void *)&CEN_UARThelper, 0, sizeof(CEN_UARThelper));
  memset((void *)&PER1_UARThelper, 0, sizeof(PER1_UARThelper));
  memset((void *)&PER2_UARThelper, 0, sizeof(PER2_UARThelper));

  CEN_UARThelper.usart = UART4;
  USART_INIT(UART4, 1, A, 0, A, 1, UART_SPEED); //CEN
  USART_INIT_HANDLER(UART4);

  PER1_UARThelper.usart = USART2;
  PER2_UARThelper.usart = USART3;
  USART_INIT(USART2, 1, A, 2, A, 3, UART_SPEED); //PER1
  USART_INIT_HANDLER(USART2);
  USART_INIT(USART3, 1, D, 8, D, 9, UART_SPEED); //PER2
  USART_INIT_HANDLER(USART3);

  //ADC sens
  GPIO_INIT(B, 1, OUT, NOPULL);

  //Watchdog
  GPIO_INIT(E, 8, OUT, NOPULL);
  wdogAssert(1);

  //Polar1/2
  PIN_SET(E, 14, ((cfg.polar>>0)&1));//Polar1
  GPIO_INIT(E, 14, OUT, NOPULL);
  PIN_SET(E, 15, ((cfg.polar>>1)&1));//Polar2
  GPIO_INIT(E, 15, OUT, NOPULL);

  //PWM_STAT and other PWM status lines
  GPIO_INIT(A, 12, IN, UP);
  GPIO_INIT(B, 3, IN, UP);
  GPIO_INIT(B, 9, IN, UP);
  GPIO_INIT(C, 14, IN, UP);
  GPIO_INIT(C, 15, IN, UP);
  GPIO_INIT(E, 13, IN, UP);

  {
    CEN_SPIhelper.moduleNum = 0;
    CEN_SPIhelper.radioModule = &radioModule[0];
    CEN_SPIhelper.master = 0;

    //Remote radio chip connected (master/slave):
    CEN_SPIhelper.connDev.devID  = 0x0;
    CEN_SPIhelper.connDev.manID  = 0x0;
    CEN_SPIhelper.connDev.prodID = 0x0;

    //Radio SPI
    SPI_INIT_HANDLER(SPI1);
    SPI_INIT_MODULE(radioModule[0].spi,
                    SPI1, 2,
                    A, 5,
                    A, 6,
                    A, 7,
                    8, 32);
    if(!(cfg.proto&0x1))
      {
        SPI_CS_INIT_MODULE(radioModule[0].spi, A, 15);
      }
    else
      {
        SPI_CS_INIT_MODULE(radioModule[0].spi, C, 1);
      }

    SETUP_EXTI(B, 4, EXTI4_IRQn, radioIntIRQ);
  }

  portData[CEN_PORT].kond_pom = cfg.kondPom;
  portData[CEN_PORT].urzadzenie = URZADZENIE;

  i2cInit(I2C1);
  i2cInit(I2C2);

  if(cfg.flags&FLAGS_POWER_CYCLE_VARI_MASK)
    {
      GPIO_INIT(E, 5, OUT, NOPULL);
      GPIO_WriteBit(GPIOE, GPIO_Pin_5, 1);
      mdelay(100);
      GPIO_WriteBit(GPIOE, GPIO_Pin_5, 0);
    }
  CONFIG_PRINT();
}

#define PLL_48 0
#define PLL_44 1
static const uint8_t config1[2][2][8]=
{
  {
    [PLL_48] = {0x01,6,0x09,0xb4,0x07,0x02,0x50,0x40},
    [PLL_44] = {0x01,6,0x09,0xb4,0x05,0x02,0x50,0x40},
  },
  {
    [PLL_48] = {0x01,6,0x09,0xB4,0x07,0x02,0x50,0x40},
    [PLL_44] = {0x01,6,0x09,0xB4,0x06,0x02,0x50,0x40},
  }
};
static const uint8_t config2[2][2][18]=
{
  {
    [PLL_48] = {0x10,16,0x00,0x00,0x00,0x00,0x6d,0x02,0x00,0x00,0x80,0x01,0xaa,0x62,0xfa,0x01,0xf2,0x44},
    [PLL_44] = {0x10,16,0x00,0x00,0x00,0x00,0x6d,0x02,0x00,0x00,0x04,0x00,0x1b,0x24,0xfa,0x01,0xf2,0x44},
  },
  {
    [PLL_48] = {0x10,16,0x00,0x00,0x00,0x00,0x6D,0x02,0x00,0x00,0x50,0x00,0xA2,0x82,0xEA,0x61,0xA2,0x64},
    [PLL_44] = {0x10,16,0x00,0x00,0x00,0x00,0x6D,0x02,0x00,0x00,0x01,0x00,0x02,0x01,0xEA,0x61,0xA2,0x64},
  },
};


int pll_set_retry(int audioOutputType, int force, int retryNUm)
{
  int status = pll_set(audioOutputType, force);

  while(retryNUm > 0 && status < 0)
    {
      retryNUm--;
      mdelay(200);
      status = pll_set(audioOutputType, force);
    }
  return status;
}

static int lastPLLSet = -1;
int pll_set(int audioOutputType, int force)
{
  if(cfg.flags&FLAGS_OMIT_PLL_MASK)
    return 1;

  if(cfg.fout22 && !force)
    return 1;
  int status = 1;
  switch(audioOutputType)
    {
    case 0x01:
    case 0x0A:
    case 0x0C:
    case 0x11:
    case 0x13:
      if(lastPLLSet != PLL_44)
        {
          dprintf(LL_INFO, "Setting PLL freq to 22.5792MHz\n");
          if(status>0) status = i2cWrite((cfg.plli2c==1)?I2C1:I2C2, 0x65, (void*)config1[cfg.ckin141?1:0][PLL_44], sizeof(config1[0][PLL_44]));
          if(status>0) status = i2cWrite((cfg.plli2c==1)?I2C1:I2C2, 0x65, (void*)config2[cfg.ckin141?1:0][PLL_44], sizeof(config2[0][PLL_44]));
          if(status>0) lastPLLSet = PLL_44;
        }
      else
        {
          dprintf(LL_INFO, "PLL freq already set to 22.5792MHz\n");
        }
      break;
    case 0x02:
    case 0x0B:
    case 0x0D:
    case 0x12:
    case 0x14:
      if(lastPLLSet != PLL_48)
        {
          dprintf(LL_INFO, "Setting PLL freq to 24.576MHz\n");
          if(status>0) status = i2cWrite((cfg.plli2c==1)?I2C1:I2C2, 0x65, (void*)config1[cfg.ckin141?1:0][PLL_48], sizeof(config1[0][PLL_48]));
          if(status>0) status = i2cWrite((cfg.plli2c==1)?I2C1:I2C2, 0x65, (void*)config2[cfg.ckin141?1:0][PLL_48], sizeof(config2[0][PLL_48]));
          if(status>0) lastPLLSet = PLL_48;
        }
      else
        {
          dprintf(LL_INFO, "PLL freq already set to 24.576MHz\n");
        }
      break;
    case 0x00:
      lastPLLSet = -1;
      dprintf(LL_INFO, "PLL reset.\n");
      break;
    default:
      dprintf(LL_INFO, "Wrong frequency chosen\n");

    }
  if(status <= 0)
    dprintf(LL_ERROR, "PLL error (%d)!\n", -status);

  return status;
}

static int wdogActive = 0;
void wdogAssert(int value)
{
  if(wdogActive)
    {
      GPIO_WriteBit(GPIOE, GPIO_Pin_8, value);
    }
  else
    {
      GPIO_WriteBit(GPIOE, GPIO_Pin_8, 1);
    }
}

void ampEnable(int enable)
{
  if(!cfg.no_rst_amp)
    {
      GPIO_WriteBit(GPIOA, GPIO_Pin_11, enable);
    }
}

void powerON1(void)
{
  //Activate watchdog
  wdogActive = 1;
}

void powerON2(void)
{
  //SPK SW
  GPIO_WriteBit(GPIOA, GPIO_Pin_9, 1);
}

void powerON3(void)
{
  //POWER_ON pulse
  GPIO_WriteBit(GPIOC, GPIO_Pin_11, 0);
  mdelay(5);
  GPIO_WriteBit(GPIOC, GPIO_Pin_11, 1);
}


void powerON4(void)
{
  //RST_AMP
  GPIO_WriteBit(GPIOA, GPIO_Pin_11, 1);
}

void powerOFF1(void)
{
  ampEnable(0);
}

void powerOFF2(void)
{
  //Disable watchdog
  wdogActive = 0;
}

void powerOFF3(void)
{
  //SPK SW
  GPIO_WriteBit(GPIOA, GPIO_Pin_9, 0);
}


//radio interrupts
void radioIntIRQ(void)
{
  // void EXTI0_IRQHandler(void)
  if (EXTI_GetITStatus(EXTI_Line4) != RESET)
    {
      SPIevent(CEN_PORT, SPI_EV_TYPE_DATA_READY);
      /* Clear the EXTI line 0 pending bit */
      EXTI_ClearITPendingBit(EXTI_Line4);
    }
}

void adcSens(int enable)
{
  if(enable)
    GPIO_WriteBit(GPIOB, GPIO_Pin_1, 1);
  else
    GPIO_WriteBit(GPIOB, GPIO_Pin_1, 0);

}


uint8_t pwmStatus(void)
{
  return 0
         | (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_12)<<0)
         | (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_3)<<1)
         | (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_9)<<2)
         | (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_14)<<3)
         | (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_15)<<4)
         | (GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_13)<<5)
         ;
}

CONFIG_CONST;
