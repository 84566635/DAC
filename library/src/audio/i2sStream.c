#include <project.h>
#include <common.h>
#include <stdio.h>
#include <string.h>
#include <misc.h>
#include <startup.h>

#include STMINCP()
#include STMINCS(_gpio)
#include STMINCS(_rcc)
#include STMINCS(_sai)
#include STMINCS(_spi)
#include STMINCS(_dma)

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>

#include <audioTypes.h>

#include <bqueue.h>
#include <convert.h>

#include <i2sStream.h>
#include <i2sConfig.h>
#include <saihwInit.h>
#include <wm8805.h>
#include <debug.h>


void inputAudioI2S_IRQHandler(void);

#define I2S_IN_BUFF_SIZE 256
#define I2S_IN_BUFF_SIZE_BYTES I2S_IN_BUFF_SIZE*sizeof(int)
#define Q_TAKE_INIT_SIZE 6
#define QUEUE_OVERFLOWING_COUNT 20

inputLocalStream_t inputLocalStream;

//external Variables:   --------------------------------------
extern constDMAsettings dmaStreamconstSettings[];

//"working on" Variables:   --------------------------------------
/* I2S peripheral global pointer */
void Audio_MAL_I2S_IRQHandler(void);


inline void configureI2S_input(SPI_TypeDef *SPIx, uint8_t *audioOutBuff, uint8_t *secAudioOutBuff, uint32_t numberOfTransfers)
{
  /* Configure the I2S peripheral */
  I2S_AudioInterfaceInit( SPIx );

  /* Play on */
  I2S_AudioPlay( SPIx, (uint32_t)audioOutBuff, (uint32_t)secAudioOutBuff, numberOfTransfers );
}


#define SPIx_AUDIO_STREAM SPI3

#define I2S_START   1
#define I2S_RESTART 2
#define I2S_RUNNING 3
static int i2sinit = I2S_START;
void i2sResync(void)
{
  if(i2sinit == I2S_RUNNING)
    i2sinit = I2S_RESTART;
}


void __attribute__((weak))I2STrimm(unsigned int count)
{
  if(i2sinit == I2S_RUNNING && I2S_AudioIsFER(SPI3))
    i2sinit = I2S_RESTART;

  switch(i2sinit)
    {
    case I2S_RESTART:
    {
      //Stop I2S

      I2S_AudioStop(SPI3);
    }
    case I2S_START:
    {

      bBuffer_t *buf1 = NULL;
      bBuffer_t *buf2 = NULL;

      buf1 = bAlloc(I2S_IN_BUFF_SIZE_BYTES);
      buf1->size = I2S_IN_BUFF_SIZE_BYTES;

      buf2 = bAlloc(I2S_IN_BUFF_SIZE_BYTES);
      buf2->size = I2S_IN_BUFF_SIZE_BYTES;

      if ( buf1 && buf2 )
        configureI2S_input(SPIx_AUDIO_STREAM, buf1->data, buf2->data, I2S_IN_BUFF_SIZE_BYTES / sizeof(short) );

      i2sinit = I2S_RUNNING;
    }
    break;
    }
}


static uint8_t i2sBuf[32];
static uint64_t i2sPeriod = 1;
void printi2s(int *bufSize, char *bufPtr);
void printi2s(int *bufSize, char *bufPtr)
{
  int i;
  DPRINTF("is2: ");
  for(i = 0; i < sizeof(i2sBuf); i++)
    {
      if((i%8) == 0)	DPRINTF(" ");
      DPRINTF("%02x", i2sBuf[i]);
    }
  DPRINTF(" period = %uns freq = %uHz"CLEAR_LINE"\n", (uint32_t)(i2sPeriod/1000), i2sGetFreq());
}


#define SAMPLE_PERIOD(arg_freq) (((I2S_IN_BUFF_SIZE/2)*1000000)/(arg_freq))
#define CASE_RANGE(arg_freq) SAMPLE_PERIOD(arg_freq)-((SAMPLE_PERIOD(arg_freq)*2)/100) ... SAMPLE_PERIOD(arg_freq)+((SAMPLE_PERIOD(arg_freq)*2)/100)
int i2sGetFreq(void)
{
  switch(i2sPeriod/1000)
    {
    case CASE_RANGE(44100):
      return 44100;
    case CASE_RANGE(48000):
      return 48000;
    case CASE_RANGE(88200):
      return 88200;
    case CASE_RANGE(96000):
      return 96000;
    case CASE_RANGE(176400):
      return 176400;
    case CASE_RANGE(192000):
      return 192000;
    }
  return 0;
}

static uint64_t lastUptime = 0ULL;
void AUDIO_TransferComplete_CallBack(uint32_t *replaceBufforAdr, uint32_t Size, signed portBASE_TYPE *xHigherPriorityTaskWoken)
{
  /* Replace receive buffer */
  uint64_t uptime = getUptimePrecise();
  i2sPeriod = uptime - lastUptime;
  lastUptime = uptime;

  bBuffer_t *buf = NULL;
  buf = bBufferFromData( (void *)*replaceBufforAdr );

  if(streamCaptureBuffer && !(streamCaptureBuffer->offset == streamCaptureBuffer->size))
    {
      int copySize = streamCaptureBuffer->size - streamCaptureBuffer->offset;
      if(copySize > buf->size)
        copySize = buf->size;
      if(copySize)
        memcpy(&(streamCaptureBuffer->data[streamCaptureBuffer->offset]), &(buf->data[buf->offset]), copySize);
      streamCaptureBuffer->offset += copySize;
      streamCaptureBufferFormat = IF_16L8L8N16P8P8N;
    }


  memcpy(i2sBuf, buf->data, sizeof(i2sBuf));
  if (bEnqueue(&inQueueI2S, buf) < 0)
    bFree(buf);
  buf = NULL;

  buf = bAlloc(I2S_IN_BUFF_SIZE_BYTES);
  buf->size = I2S_IN_BUFF_SIZE_BYTES;

  *replaceBufforAdr = (uint32_t)bDataFromBuffer(buf);

}



/**
 * @brief  Manages the DMA FIFO error interrupt.
 * @param  None
 * @retval None
 */
void I2S_AUDIO_Error_CallBack(void *pData)
{
  I2S_Cmd(SPIx_AUDIO_STREAM, DISABLE);
  /* Stop the program */
  int s = 1;
  while (s);
  /* could also generate a system reset to recover from the error */
}
