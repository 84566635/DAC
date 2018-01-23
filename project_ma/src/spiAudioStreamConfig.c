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
#include <isrHandlersPrototypes.h>
#include <spiAudioStreamConfig.h>


void spiSetupDmaForStream(void)
{


  (void)SPI_NUM_MACRO(SPIx_AUDIO_STREAM)->DR; //concatenate SPI with number defined under SPIx_AUDIO_STREAM
  while ( DMA_GetCmdStatus(OLINUXINO_DMAx_STREAMx) );   // wait for dma to flush fifo

  bBuffer_t *buf1 = NULL;
  bBuffer_t *buf2 = NULL;

  buf1 = bAlloc(SPI_TOTAL_BUFFER_SIZE);
  buf1->size = SPI_BUFFER_SIZE;

  buf2 = bAlloc(SPI_TOTAL_BUFFER_SIZE);
  buf2->size = SPI_BUFFER_SIZE;

  if ( buf1 && buf2 )
    {

      DMA_SetCurrDataCounter(OLINUXINO_DMAx_STREAMx, SPI_TOTAL_BUFFER_SIZE/SPI_CHUNK_SIZE);

      DMA_MemoryTargetConfig(OLINUXINO_DMAx_STREAMx, (uint32_t)buf1->data, DMA_Memory_0);
      DMA_DoubleBufferModeConfig(OLINUXINO_DMAx_STREAMx, (uint32_t)buf2->data, 0 );    // zero stands for first used buffe

      DMA_Cmd(OLINUXINO_DMAx_STREAMx, ENABLE);
    }
}

void DMA_SPI_audioStream_ISR(void)
{
  static signed portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

  /* Transfer complete interrupt */
  if (DMA_GetFlagStatus(OLINUXINO_DMAx_STREAMx, OLINUXINO_DMA_FLAG_TCIFx) != RESET)
    {
      ///////////////////////////////////////////////////////////////////////////////////////////////////////
      uint32_t currMem = DMA_GetCurrentMemoryTarget(OLINUXINO_DMAx_STREAMx);
      if (currMem)
        {
          uint32_t replaceAddr = OLINUXINO_DMAx_STREAMx->M0AR;
          spiAudioStreamTC_Callback( &replaceAddr , OLINUXINO_DMAx_STREAMx->NDTR, &xHigherPriorityTaskWoken);
          DMA_MemoryTargetConfig(OLINUXINO_DMAx_STREAMx, replaceAddr, DMA_Memory_0);
        }
      else
        {
          uint32_t replaceAddr = OLINUXINO_DMAx_STREAMx->M1AR;
          spiAudioStreamTC_Callback( &replaceAddr , OLINUXINO_DMAx_STREAMx->NDTR, &xHigherPriorityTaskWoken);
          DMA_MemoryTargetConfig(OLINUXINO_DMAx_STREAMx, replaceAddr, DMA_Memory_1);
        }

      /* Clear the Interrupt flag */
      DMA_ClearFlag(OLINUXINO_DMAx_STREAMx, OLINUXINO_DMA_FLAG_TCIFx);
    }

  portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

