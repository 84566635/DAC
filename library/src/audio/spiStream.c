#include <project.h>
#include <common.h>
#include <stdio.h>
#include <string.h>
#include <misc.h>
#include <startup.h>
#include <hwProfile.h>


#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>
#include <hwCommunicator.h>
#include <hwCommunicatorUARThelper.h>
#include <hwCommunicatorSPIhelper.h>

#include STMINCP()
#include STMINCS(_gpio)
#include STMINCS(_rcc)
#include STMINCS(_sai)
#include STMINCS(_dma)
#include STMINCS(_hwInit)

#include <bqueue.h>
#include <convert.h>

#include <audioTypes.h>
#include <saihwInit.h>
#include <spiAudioStreamConfig.h>
#include <spiStream.h>

#include <isrHandlersPrototypes.h>


typedef struct
{
  uint8_t controlPattern_1;
  uint8_t controlPattern_2;
  uint8_t controlPattern_3;
  uint8_t controlPattern_4;
  uint8_t audioRate;
  uint8_t sizeOfSample;
  uint8_t rest[26];
} audioHeader_t;


static int mapFreq(int mapFreq)
{
  switch(mapFreq)
    {
    case 44:
      return 44100;
    case 48:
      return 48000;
    case 88:
      return 88200;
    case 96:
      return 96000;
    case 176:
      return 176400;
    case 192:
      return 192000;
    }
  return 48000;
}
static input_format_e mapFormat(int mapbit)
{
  switch(mapbit)
    {
    case 0:
      return IF_16L16P;
    case 1:
      return IF_16L16P;
    case 2:
      return IF_24L24P;
    case 3:
      return IF_24L8N24P8N;
    }
  return IF_16L16P;
}
static bQueue_t *queue = NULL;
static int spiNum = 0;
static int frequencies[3] = {0,0,0};
static input_format_e formats[3] = {0,0,0};
void spiAudioStreamTC_Callback(uint32_t *replaceBufforAdr, uint32_t Size, signed portBASE_TYPE *xHigherPriorityTaskWoken)
{
  /* Replace receive buffer */
  bBuffer_t *buf = bBufferFromData((uint8_t *)*replaceBufforAdr);
  audioHeader_t *header = (void*)buf->data;
  int sampleSize = header->sizeOfSample;
  int rate       = header->audioRate;
  if(spiNum > 0 && spiNum < 3)
    {
      int send = 0;
      if(frequencies[spiNum] != mapFreq(rate)) send = 1;
      if(formats[spiNum] != mapFormat(sampleSize)) send = 1;
      frequencies[spiNum] = mapFreq(rate);
      formats[spiNum] = mapFormat(sampleSize);
      if(send)
        formatChange((spiNum==1)?INPUT_SPI1:INPUT_SPI2, formats[spiNum], frequencies[spiNum]);
    }
  if(queue)
    {
      if (bEnqueue(queue, buf) >= 0)
        buf = bAlloc(SPI_TOTAL_BUFFER_SIZE);
    }
  buf->size = SPI_BUFFER_SIZE;
  buf->offset = SPI_HEADER_TAIL_SIZE;
  *replaceBufforAdr = (uint32_t)bDataFromBuffer(buf);
}

static int init = 0;

static void SPIenable(int spi, int enable)
{
  if(!init)
    {
      spiSetupDmaForStream();
      init = 1;
    }
  taskENTER_CRITICAL();
  if(enable)
    {
      switch(spi)
        {
        case 1:
          PIN_SET(E, 10, 1);
          queue = &inQueueSPI1;
          spiNum = 1;

          PIN_SET(E, 9, 0);
          break;
        case 2:
          PIN_SET(E, 9, 1);
          queue = &inQueueSPI2;
          spiNum = 2;
          PIN_SET(E, 10, 0);
          break;
        default:
          PIN_SET(E, 9, 1);
          PIN_SET(E, 10, 1);
          //          queue = NULL;
          //          spiNum = 0;
          break;
        }
    }
  else
    {
      PIN_SET(E, 9, 1);
      PIN_SET(E, 10, 1);
      queue = NULL;
      spiNum = 0;
    }
  taskEXIT_CRITICAL();
}

void SPI1Trimm(unsigned int count)
{
  int limits[4] = {QUEUE_SPI1_LIMITS};
  if(count < limits[1])
    {
      //Enable stream
      SPIenable(1, 1);
    }
  if(count >= limits[2])
    {
      //Disable stream
      SPIenable(1, 0);
    }
}

void SPI2Trimm(unsigned int count)
{
  int limits[4] = {QUEUE_SPI2_LIMITS};
  if(count < limits[1])
    {
      //Enable stream
      SPIenable(2, 1);
    }
  if(count >= limits[2])
    {
      //Disable stream
      SPIenable(2, 0);
    }
}
