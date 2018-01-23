
#ifndef I2S_STREAM_H_
#define I2S_STREAM_H_

uint32_t i2sClose(outputStream_t *outputStream, taskSync_t *taskSync);
uint32_t i2sOperate(outputStream_t *outputStream, taskSync_t *taskSync);
uint32_t i2sTransInit(outputStream_t *outputStream, taskSync_t *taskSync);
int i2sGetFreq(void);

void configureI2S_input(SPI_TypeDef *SPIx, uint8_t *audioOutBuff, uint8_t *secAudioOutBuff, uint32_t buffSize);

typedef struct
{
  xSemaphoreHandle semaphore;  // for synchronization with end of SPI transfer
  constDMAsettings *hwDma;
  audioRate_t audioRate;    //rate read from spiFrame
  dataSize_t dataFormat;    // dataSize read from spiFrame: 16b, or 24b (goes to uin32_t)
  SPI_TypeDef *SPIx;

  queue_t *qInsert;
  queue_t *qTake;
  uint32_t statRcv;

} inputLocalStream_t;

#endif
