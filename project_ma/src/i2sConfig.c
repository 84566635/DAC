#include <project.h>
#include <hwProfile.h>
#include <common.h>
#include <string.h>
#include <misc.h>
#include <startup.h>

#include STMINCP()
#include STMINCS(_gpio)
#include STMINCS(_rcc)
#include STMINCS(_sai)
#include STMINCS(_dma)
#include STMINCS(_spi)

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>
#include <bqueue.h>

#include <audioTypes.h>
#include <audioQueue.h>
#include <i2sConfig.h>
#include <isrHandlersPrototypes.h>
#include <messageProcessorMADO.h>
#include "wm8805.h"

static void I2S_DataStreamInit(SPI_TypeDef *SPIx, constDMAsettings *hwDma);

extern constDMAsettings dmaStreamconstSettings[];
constDMAsettings *constGlobHwDma = NULL;

/**
  * @brief  Initializes the audio interface (I2S)
  * @note   This function not handles clock configuration very well yet
  * @param  SPIx: Selects one of two possible SPIx: 2,3.
  * @param  AudioFreq: Audio frequency to be configured for the I2S peripheral.
  * @retval None
  */
void I2S_AudioInterfaceInit( SPI_TypeDef *SPIx )
{
  I2S_InitTypeDef I2S_InitStructure;

  /* Enable the CODEC_I2S peripheral clock */
  switch ((uint32_t)SPIx)
    {
    case (uint32_t)SPI2:
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
      massert(0); // no I2S2_DMA position in table with constant DMA settings
      constGlobHwDma = &(dmaStreamconstSettings[I2S3_DMA]);  // for dma settings
      break;
    case (uint32_t)SPI3:
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);
      constGlobHwDma = &(dmaStreamconstSettings[I2S3_DMA]);  // for dma settings
      break;
    default:
      //perph not supported
      massert(0);
    }


  /* CODEC_I2S peripheral configuration */
  SPI_I2S_DeInit(SPIx);
  I2S_InitStructure.I2S_AudioFreq = 48000;  // not used in slaveRX
  I2S_InitStructure.I2S_Standard = I2S_Standard_Phillips;
  I2S_InitStructure.I2S_DataFormat = I2S_DataFormat_32b;
  I2S_InitStructure.I2S_CPOL = I2S_CPOL_Low;
  I2S_InitStructure.I2S_Mode = I2S_Mode_SlaveRx;
  I2S_InitStructure.I2S_MCLKOutput = I2S_MCLKOutput_Disable;

  /* Initialize the I2S peripheral with the structure above */
  I2S_Init(SPIx, &I2S_InitStructure);
  (void)SPI_I2S_ReceiveData(SPIx);

  /* I2S data transfer preparation:
  Prepare the Media to be used for the audio transfer from memory to I2S peripheral */
  //Audio_MAL_Init();
  I2S_DataStreamInit(SPIx, constGlobHwDma);
}

/**
  * @brief  Initializes data transfer from I2Sx: x = {2,3}
  * @param  None
  * @retval None
  */
static int registered = 0;
static void I2S_DataStreamInit(SPI_TypeDef *SPIx, constDMAsettings *hwDma)
{
  massert(SPIx);
  DMA_InitTypeDef DMA_InitStructure;

  /* Enable the DMA clock */
  RCC_AHB1PeriphClockCmd(hwDma->RCC_AHB_DMA_Clock, ENABLE);

  /* Configure the DMA Stream */
  DMA_Cmd(hwDma->DMAy_Streamx, DISABLE);
  DMA_DeInit(hwDma->DMAy_Streamx);
  /* Set the parameters to be configured */
  DMA_InitStructure.DMA_Channel = hwDma->DMA_Channel_x;
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) & (SPIx->DR);
  DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)0;      /* This field will be configured in play function */
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
  DMA_InitStructure.DMA_BufferSize = (uint16_t)0xFFFE;      /* This field will be configured in play function */
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_1QuarterFull;
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
  DMA_Init(hwDma->DMAy_Streamx, &DMA_InitStructure);

  DMA_DoubleBufferModeCmd(hwDma->DMAy_Streamx, ENABLE);

  DMA_ITConfig(hwDma->DMAy_Streamx, DMA_IT_TC, ENABLE);
  // DMA_ITConfig(hwDma->DMAy_Streamx, DMA_IT_HT, ENABLE);
  //DMA_ITConfig(hwDma->DMAy_Streamx, DMA_IT_TE | DMA_IT_FE | DMA_IT_DME, ENABLE);
  DMA_ITConfig(hwDma->DMAy_Streamx, DMA_IT_TE | DMA_IT_DME, ENABLE);

  if(!registered)
    {
      registerIRQ(hwDma->DMAx_Streamx_IRQn, inputAudioI2S_IRQHandler);
      NVIC_SetPriority(hwDma->DMAx_Streamx_IRQn, configMAX_SYSCALL_INTERRUPT_PRIORITY);
      NVIC_EnableIRQ(hwDma->DMAx_Streamx_IRQn);
      registered = 1;
    }
  SPI_I2S_DMACmd(SPIx, SPI_I2S_DMAReq_Rx, ENABLE);

}

void I2S_AudioPlay(SPI_TypeDef *SPIx, uint32_t Addr, uint32_t Addr2, uint32_t Size)
{
  /* Configure the buffer address and size */
  //Twice the DMA transfers for 32b in I2S3
  DMA_SetCurrDataCounter(constGlobHwDma->DMAy_Streamx, Size);

  DMA_MemoryTargetConfig(constGlobHwDma->DMAy_Streamx, Addr, DMA_Memory_0);
  //DMA_MemoryTargetConfig(constGlobHwDma->DMAy_Streamx, Addr2, DMA_Memory_0);
  DMA_DoubleBufferModeConfig(constGlobHwDma->DMAy_Streamx, (uint32_t)Addr2, 0 );    // zero stands for first used buffer

  /* Enable the I2S DMA Stream*/
  DMA_Cmd(constGlobHwDma->DMAy_Streamx, ENABLE);

  /* If the I2S peripheral is still not enabled, enable it */

  //Wait for the beginning of WS high
  while(GPIO_ReadInputDataBit(I2S_INPUT_WS_GPIO, I2S_INPUT_WS_PIN) == 1);
  while(GPIO_ReadInputDataBit(I2S_INPUT_WS_GPIO, I2S_INPUT_WS_PIN) == 0);

  if ((SPIx->I2SCFGR & I2S_ENABLE_MASK) == 0)
    {
      I2S_Cmd(SPIx, ENABLE);
    }
}

void I2S_AudioPause(SPI_TypeDef *SPIx)
{
  I2S_Cmd(SPIx, DISABLE);
}

void I2S_AudioStop(SPI_TypeDef *SPIx)
{
  DMA_Cmd(constGlobHwDma->DMAy_Streamx, DISABLE);
  I2S_Cmd(SPIx, DISABLE);
  while ((SPIx->I2SCFGR & I2S_ENABLE_MASK) != 0);
  bBuffer_t *buf = NULL;
  buf = bBufferFromData( (void *)constGlobHwDma->DMAy_Streamx->M0AR);
  bFree(buf);
  buf = bBufferFromData( (void *)constGlobHwDma->DMAy_Streamx->M1AR);
  bFree(buf);
  (void)SPI_I2S_ReceiveData(SPIx);
}

int I2S_AudioIsFER(SPI_TypeDef *SPIx)
{
  return !!(SPIx->SR & (SPI_I2S_FLAG_TIFRFE | SPI_I2S_FLAG_OVR | I2S_FLAG_UDR));
}

static int rxChannel[2] = {-1, -1};
void i2sSelectSource(int wmNum, int input)
{
  if(rxChannel[wmNum] != input)
    {
      spdifChip_t wmChip = wmNum?WM8805_2:WM8805_1;
      WM88XXSetReceiverChannel(wmChip, input);
      rxChannel[wmNum] = input;
    }
}


//////////////////////// ISR ////////////////////////////

void inputAudioI2S_IRQHandler(void)
{
  static signed portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

  /* Transfer complete interrupt */
  if (DMA_GetFlagStatus(constGlobHwDma->DMAy_Streamx, constGlobHwDma->dmaFlagTCIF) != RESET)
    {
      ///////////////////////////////////////////////////////////////////////////////////////////////////////
      uint32_t currMem = DMA_GetCurrentMemoryTarget(constGlobHwDma->DMAy_Streamx);
      //#warning Implement idea: use 2 element tables for MxAR and DMA_Memory_x to avoid if conditons
      if (currMem)
        {
          uint32_t replaceAddr = constGlobHwDma->DMAy_Streamx->M0AR;
          AUDIO_TransferComplete_CallBack( &replaceAddr , constGlobHwDma->DMAy_Streamx->NDTR, &xHigherPriorityTaskWoken);
          DMA_MemoryTargetConfig(constGlobHwDma->DMAy_Streamx, replaceAddr, DMA_Memory_0);
        }
      else
        {
          uint32_t replaceAddr = constGlobHwDma->DMAy_Streamx->M1AR;
          AUDIO_TransferComplete_CallBack( &replaceAddr , constGlobHwDma->DMAy_Streamx->NDTR, &xHigherPriorityTaskWoken);
          DMA_MemoryTargetConfig(constGlobHwDma->DMAy_Streamx, replaceAddr, DMA_Memory_1);
        }

      /* Clear the Interrupt flag */
      DMA_ClearFlag(constGlobHwDma->DMAy_Streamx, constGlobHwDma->dmaFlagTCIF);
    }

  /* Half Transfer complete interrupt */
  // if (DMA_GetFlagStatus(constGlobHwDma->DMAy_Streamx, AUDIO_MAL_DMA_FLAG_HT) != RESET)
  //   {
  //      Manage the remaining file size and new address offset: This function
  //        should be coded by user (its prototype is already declared in stm32f4_discovery_audio_codec.h)
  //     AUDIO_HalfTransfer_CallBack((uint32_t)0, 1231);

  //     /* Clear the Interrupt flag */
  //     DMA_ClearFlag(constGlobHwDma->DMAy_Streamx, AUDIO_MAL_DMA_FLAG_HT);
  //   }


  /* FIFO Error interrupt */
  if ((DMA_GetFlagStatus(constGlobHwDma->DMAy_Streamx, constGlobHwDma->dmaFlagTEIF) != RESET) ||
      //(DMA_GetFlagStatus(constGlobHwDma->DMAy_Streamx, AUDIO_MAL_DMA_FLAG_FE) != RESET) ||
      (DMA_GetFlagStatus(constGlobHwDma->DMAy_Streamx, constGlobHwDma->dmaFlagDMEIF) != RESET))

    {
      // Manage the error generated on DMA FIFO: This function
      //   should be coded by user (its prototype is already declared in stm32f4_discovery_audio_codec.h)
      I2S_AUDIO_Error_CallBack((uint32_t *)NULL);

      /* Clear the Interrupt flag */
      DMA_ClearFlag(constGlobHwDma->DMAy_Streamx, constGlobHwDma->dmaFlagTEIF |
                    //AUDIO_MAL_DMA_FLAG_FE |
                    constGlobHwDma->dmaFlagDMEIF);
    }

  portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}
