#ifndef AUDIO_TYPES_H_
#define AUDIO_TYPES_H_

// include audioStreamDefs.h


#define SPI_HEADER_TAIL_SIZE        32
#define SPI_BUFFER_SIZE             2048
#define AUDIO_BUFFER_SIZE           2048
#define SPI_TOTAL_BUFFER_SIZE       (2048+2*32)


#define DEFAULT_FRAME_LEVEL         40
#define DEFAULT_FRAME_RATE          48

typedef enum
{
  SAI_PLL = 0,
  I2S_PLL,
  EXTERNAL,
} saiClkSource_t;

typedef enum
{
  SET_I2S = 0,
  SET_DSP,
} audioI2SFormat_t;


typedef enum
{
  TEST = 0,
  SPI,
  USB,
  I2S,
  NO_INPUT,
} streamSource_t;


//system interface commands
typedef enum
{
  STOP,
  PAUSE,
  RESUME,
  CHANGE_INPUT,
} audioInterCmd_t;

//system interface commands
typedef enum
{
  STOP_STATE,
  PAUSE_STATE,
  ACTIVE_STATE,
} audioCoreState_t;


typedef enum
{
  CONTROL_PATTERN_1 = 0x11,
  CONTROL_PATTERN_2 = 0x22,
  CONTROL_PATTERN_3 = 0x33,
  CONTROL_PATTERN_4 = 0x44,
} frameControlPattern_t;


typedef enum
{
  _32_kHz = 0,
  _40_kHz,
  _44_kHz,
  _48_kHz,
  _88_kHz,
  _96_kHz,
  _176_kHz,
  _192_kHz,
  _EXTERNAL,
} audioRate_t;


typedef enum
{
  _16BIT = 0,
  _16BITE,
  _24BIT,
  _32BIT,
} dataSize_t;

typedef enum
{
  MONO = 1,
  STEREO = 2,
  _4 = 4,
} audioChannels_t;

typedef enum
{
  NOT_USING_PRECONF = 0,
  SAI_TEST1,
  SAI_EXTERNAL_CLK_I2S,
} SAI_setupConfiguration;

typedef struct __attribute__ ((__packed__))
{
  saiClkSource_t clkSrc;
  audioRate_t clkFreq;
  audioI2SFormat_t audioI2SFormat;
} audioFormatMsg_t;

typedef struct __attribute__ ((__packed__))
{
  audioRate_t audioRate;    //rate read from spiFrame
  dataSize_t sizeOfSample;    // dataSize read from spiFrame: 16b, or 24b (goes to uin32_t)
  audioChannels_t chNum;
  uint32_t slotsNum;
  SAI_setupConfiguration usePreconfiguredSettings;
} audioFormat_t;

typedef struct
{
  audioInterCmd_t cmd;
  streamSource_t input;
  audioFormatMsg_t audioFormat;
} audioCoreMsg_t;


typedef struct
{
  uint8_t rest[32];
} audioTail_t;

typedef struct __attribute__ ((__packed__)) audioPack_s
{
  struct audioPack_s *next;
  struct audioPack_s *prev;
  uint32_t timeStamp;
  uint16_t buff[];   //maybe size of short or int depending on audio mode 16b or 24/32b
} audioPack_t;

//defines unit for queue_t struct
#define unit_t audioPack_t
#include <audioQueue.h>

typedef struct
{
  audioRate_t audioRate;
  audioChannels_t channelNum;
  dataSize_t dataFormat;
} audioMode_t;

typedef struct
{
  queue_t qOut;
  queue_t qTrash;
  // /last element send (to Clear queue);
} outputAudioSync_t;


// typedef struct
// {
//   xSemaphoreHandle spiTransferSemaphore;  // for synchronization with end of SPI transfer
//   audioRate_t audioRate;    //rate read from spiFrame
//   dataSize_t dataFormat;    // dataSize read from spiFrame: 16b, or 24b (goes to uin32_t)
// } spiStreamControl_t;


typedef struct
{
  //xSemaphoreHandle spiTransferSemaphore;  // for synchronization with end of SPI transfer
  audioRate_t audioRate;    //rate read from spiFrame
  dataSize_t dataFormat;    // dataSize read from spiFrame: 16b, or 24b (goes to uin32_t)
} inputStream_t;


typedef struct __attribute__ ((__packed__))
{
  DMA_Stream_TypeDef *DMAy_Streamx;
  uint32_t DMA_Channel_x;
  uint32_t RCC_AHB_DMA_Clock;
  IRQn_Type DMAx_Streamx_IRQn;

  uint32_t dmaItTCIF;
  uint32_t dmaItTEIF;
  uint32_t dmaItFEIF;
  uint32_t dmaItDMEIF;
  uint32_t dmaFlagTCIF;
  uint32_t dmaFlagTEIF;
  uint32_t dmaFlagFEIF;
  uint32_t dmaFlagDMEIF;
} constDMAsettings;

typedef struct  __attribute__ ((__packed__))
{
  SAI_Block_TypeDef *SAI_Block_x;
  xSemaphoreHandle streamOutSemaphore;    //set to NULL in case of other synchronization element ( input stream as master )
  uint32_t fillQueueReqTreshold;
  audioFormatMsg_t audioFormat;
  queue_t *queue;
  queue_t *trash;
  audioPack_t *currentlySending;
  uint32_t statSent;
  saiClkSource_t clkSrc;
  audioRate_t clkFreq;
  audioI2SFormat_t audioI2SFormat;
} outputStream_t;


typedef struct
{
  xSemaphoreHandle semaphore;
  uint32_t semaphoreTimeout;
} taskSync_t;


typedef struct
{
  audioRate_t audioRate;
  audioChannels_t channelNum;
  dataSize_t dataFormat;

  //xQueueHandle xQueueCmd;

  //dynamic buffer size control
  uint32_t outputBufferSize;

  queue_t qOut;
  queue_t qTrash;
  //semaphore:
  uint32_t qOutTreshold;
} audioManager_t;


typedef uint32_t (*init_t)      (outputStream_t *outputStream, taskSync_t *taskSync);
typedef uint32_t (*operate_t)   (outputStream_t *outputStream, taskSync_t *taskSync);
typedef uint32_t (*close_t)     (outputStream_t *outputStream, taskSync_t *taskSync);
typedef uint32_t (*resume_t)   (outputStream_t *outputStream, taskSync_t *taskSync);
typedef uint32_t (*pause_t)     (outputStream_t *outputStream, taskSync_t *taskSync);

typedef struct
{
  init_t initCallback;
  operate_t operateCallback;
  close_t closingCallback;
  resume_t resumeCallback;
  pause_t pauseCallback;
} callbackSetHolder_t;



typedef struct
{
  //callBask functions
  init_t  initCallback;
  operate_t operateCallback;
  close_t closingCallback;
} audioCalbacks_t;


#define SIZE_OF_PACK(arg_queue)  sizeof(audioPack_t) + arg_queue->dataSize*arg_queue->bufferSize
#define SIZE_OF_AUDIO_BUFF(arg_queue)  arg_queue->dataSize*arg_queue->bufferSize
#define MALLOC_AUDIO_PACK(arg_element, arg_queue) arg_element = intSafeMalloc( sizeof(audioPack_t) + arg_queue->dataSize*arg_queue->bufferSize ); \
                                                                massert(arg_element);

#define WAIT_SYSTEM_CMD_TIMEOUT 100



#endif
