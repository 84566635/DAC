#ifndef SPI_AUDIO_STREAM_H_
#define SPI_AUDIO_STREAM_H_

typedef struct  __attribute__ ((__packed__))
{
  uint32_t spiHeaderSize;
  uint32_t spiTailSize;
  uint32_t spiBufferSizeBytes;
  uint32_t spiPayloadSizeBytes;
  uint8_t *spiBuffer;
  uint8_t dataSizeBytes;
  uint32_t waitForPacketTimeoutTicks;
  audioFormat_t audioFormat;
  // audioRate_t audioRate;
  // dataSize_t dataSize;    // set for DMA: 16b or 32b - also indicates qOut data size
  xSemaphoreHandle spiTransferSemaphore;
  uint32_t statRcv;
} spiStreamControl_t;

uint32_t spiClose(outputStream_t *outputStream, taskSync_t *taskSync);
uint32_t spiOperate(outputStream_t *outputStream, taskSync_t *taskSync);
uint32_t spiTransInit(outputStream_t *outputStream, taskSync_t *taskSync);

uint8_t startSpiStream( uint8_t *buff, uint32_t length);

uint32_t processSpiStreamAndInsert(uint8_t *buff, uint32_t length, queue_t *qOut, queue_t *qTrash, uint8_t dataSize);


#endif
