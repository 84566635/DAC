#include <project.h>
#include <common.h>
#include <startup.h>
#include <FreeRTOS.h>
#include <semphr.h>
#include STMINCP()
#include STMINCS(_rcc)
#include STMINCS(_gpio)
#include STMINCS(_usart)
#include STMINCS(_spi)
#include STMINCS(_dma)
#include STMINCS(_adc)
#include STMINCS(_hwInit)
#include <adc.h>
#include <hwCommunicator.h>
#include <hwCommunicatorSPIhelper.h>
#include <hwCommunicatorUARThelper.h>

static int spiNoIRQ = 0;
void spiIRQEnable(int enable)
{
  spiNoIRQ = !enable;
}
void __attribute__((optimize("-O3")))txSPI(void *spi, uint8_t txByte)
{
  //  while (SPI_I2S_GetFlagStatus(spi, SPI_FLAG_BSY) != RESET);
  //Discard old data
  (void)SPI_I2S_ReceiveData(spi);
  SPI_I2S_SendData(spi, txByte);
  if(!spiNoIRQ)
    {
      int spiNum = spiToSpiNum(spi);
      if (spiNum != -1)
        {
          SPI_I2S_ITConfig(spi, SPI_I2S_IT_TXE, ENABLE);
          massert(xSemaphoreTake(spiData[spiNum].xSemaphore, portMAX_DELAY ) == pdTRUE );
        }
    }
  while (SPI_I2S_GetFlagStatus(spi, SPI_FLAG_TXE) == RESET);
}
uint8_t  __attribute__((optimize("-O3")))rxSPI(void *spi)
{
  while (SPI_I2S_GetFlagStatus(spi, SPI_FLAG_RXNE) == RESET);
  //  while (SPI_I2S_GetFlagStatus(spi, SPI_FLAG_BSY) != RESET);
  return SPI_I2S_ReceiveData(spi);
}

void __attribute__((optimize("-O3")))configureDMAforSPI(void *spi, void *txBuffer, void *rxBuffer, int size)
{
  int spiNum = spiToSpiNum(spi);
  if (spiNum != -1)
    {
      //Discard old data
      (void)SPI_I2S_ReceiveData(spi);
      DMA_InitTypeDef  DMA_InitStructure;
      /* Configure DMA Initialization Structure */
      DMA_InitStructure.DMA_BufferSize = size ;
      DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable ;
      DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_1QuarterFull ;
      DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single ;
      DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
      DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
      DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
      DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) (&(spiData[spiNum].SPIx->DR)) ;
      DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
      DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
      DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
      DMA_InitStructure.DMA_Priority = DMA_Priority_High;
      /* Configure TX DMA */
      DMA_InitStructure.DMA_Channel = spiData[spiNum].txChannel ;
      DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral ;
      DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)txBuffer ;
      DMA_Init(spiData[spiNum].txStream, &DMA_InitStructure);
      /* Configure RX DMA */
      DMA_InitStructure.DMA_Channel = spiData[spiNum].rxChannel ;
      DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory ;
      DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)rxBuffer ;
      DMA_Init(spiData[spiNum].rxStream, &DMA_InitStructure);


      DMA_ITConfig(spiData[spiNum].rxStream, DMA_IT_TC, ENABLE);
      DMA_Cmd(spiData[spiNum].rxStream, ENABLE);
      DMA_Cmd(spiData[spiNum].txStream, ENABLE);

      SPI_Cmd(spiData[spiNum].SPIx, ENABLE);

      //Wait for the end of transaction
      if(spiNoIRQ)
        {
          while(!DMA_GetITStatus(spiData[spiNum].rxStream, spiData[spiNum].intMask));
          DMA_ClearITPendingBit(spiData[spiNum].rxStream, spiData[spiNum].intMask);
        }
      else
        {
          massert(xSemaphoreTake(spiData[spiNum].xSemaphore, portMAX_DELAY ) == pdTRUE );
        }

      SPI_Cmd(spiData[spiNum].SPIx, DISABLE);

      DMA_Cmd(spiData[spiNum].rxStream, DISABLE);
      DMA_Cmd(spiData[spiNum].txStream, DISABLE);
      DMA_DeInit(spiData[spiNum].txStream);
      DMA_DeInit(spiData[spiNum].rxStream);

    }
}

extern xSemaphoreHandle ADC_xSemaphore;
extern int sensADCnew;
extern int sensADC;
//Create IRQ handler for ADC
ADC_HANDLER(DMA2, 0, ADC_xSemaphore);

void ADC_hwInit(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
  GPIO_Init(GPIOC, &GPIO_InitStructure);

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

  ADC_CommonInitTypeDef ADC_CommonInitStructure;
  ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
  ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div2;
  ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
  ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
  ADC_CommonInit(&ADC_CommonInitStructure);

  /* ADC1 configuration ------------------------------------------------------*/
  ADC_InitTypeDef ADC_InitStructure;
  ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
  ADC_InitStructure.ADC_ScanConvMode = ENABLE;
  ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
  ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
  ADC_InitStructure.ADC_NbrOfConversion = 4;
  ADC_Init(ADC1, &ADC_InitStructure);


  /* ADC1 regular channel14 configuration */
  ADC_RegularChannelConfig(ADC1, ADC_Channel_10, 1, ADC_SampleTime_28Cycles);
  ADC_RegularChannelConfig(ADC1, ADC_Channel_11, 2, ADC_SampleTime_28Cycles);
  ADC_RegularChannelConfig(ADC1, ADC_Channel_12, 3, ADC_SampleTime_28Cycles);
  ADC_RegularChannelConfig(ADC1, ADC_Channel_13, 4, ADC_SampleTime_28Cycles);

  ADC_DMACmd(ADC1, ENABLE);
  ADC_DMARequestAfterLastTransferCmd(ADC1, ENABLE);

  /* Enable ADC1 */
  ADC_Cmd(ADC1, ENABLE);

  vSemaphoreCreateBinary(ADC_xSemaphore);
  massert(ADC_xSemaphore != NULL);
  massert(xSemaphoreTake(ADC_xSemaphore, portMAX_DELAY ) == pdTRUE );

  ADC_INIT_HANDLER(DMA2, 0);
}

void ADC_startConversion(uint16_t *buffer, int size)
{

  DMA_InitTypeDef       DMA_InitStructure;
  DMA_DeInit(DMA2_Stream0);
  DMA_InitStructure.DMA_Channel = ADC_Channel_0;
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;
  DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)buffer;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
  DMA_InitStructure.DMA_BufferSize = size / sizeof(uint16_t);
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
  DMA_Init(DMA2_Stream0, &DMA_InitStructure);
  DMA_ITConfig(DMA2_Stream0, DMA_IT_TC | DMA_IT_HT, ENABLE);
  DMA_Cmd(DMA2_Stream0, ENABLE);

  ADC_SoftwareStartConv(ADC1);
}



/// change port mode for cc85xx ehif:

// static void changePinMode(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, uint8_t mode)
// {
//   uint32_t mask = EHIF_SPI_HOOK->mosi.pin * EHIF_SPI_HOOK->mosi.pin;
//   APPLY_MASK(GPIOx->MODER, GPIO_Mode_AF * mask);
// }

inline void forcePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, uint8_t state)
{
  uint32_t mask = GPIO_Pin * GPIO_Pin;
  APPLY_MASK(GPIOx->MODER, 3 * mask, GPIO_Mode_OUT * mask);
  GPIO_WriteBit(GPIOx, GPIO_Pin, state);
}

inline uint8_t readPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
  uint32_t mask = GPIO_Pin * GPIO_Pin;
  APPLY_MASK(GPIOx->MODER, 3 * mask, GPIO_Mode_IN * mask);
  char status = GPIO_ReadInputDataBit(GPIOx, GPIO_Pin);
  APPLY_MASK(GPIOx->MODER, 3 * mask, GPIO_Mode_AF * mask);

  return status;
}

inline void releasePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
  uint32_t mask = GPIO_Pin * GPIO_Pin;
  APPLY_MASK(GPIOx->MODER, 3 * mask, GPIO_Mode_AF * mask);
}
