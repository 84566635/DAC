void aConvInit(void);
extern bQueue_t inQueueUSB1;
extern bQueue_t inQueueUSB2;
extern bQueue_t inQueueSPI1;
extern bQueue_t inQueueSPI2;
extern bQueue_t inQueueI2S;
extern bQueue_t inQueueTest;
extern bQueue_t outQueueSAI1;
extern bQueue_t outQueueSAI2;
typedef enum
{
  FREQ_NONE,
  FREQ_44100,
  FREQ_48000,
  FREQ_88200,
  FREQ_96000,
  FREQ_176400,
  FREQ_192000,
  FREQ_NUM,
} freq_e;

typedef enum
{
  INPUT_NONE,
  INPUT_USB1,
  INPUT_USB2,
  INPUT_SPI1,
  INPUT_SPI2,
  INPUT_SPDIF1,
  INPUT_SPDIF2,
  INPUT_TEST,
  INPUT_NUM,
} input_e;

typedef enum
{
  OUTPUT_NONE,
  OUTPUT_SAI1,
  OUTPUT_SAI2,
  OUTPUT_NUM,
} output_e;

typedef enum
{
  IF_16L16P,
  IF_16L16N16P16N,
  IF_24L24P,
  IF_24L8N24P8N,
  IF_16L8L8N16P8P8N,
  IF_NUM,
} input_format_e;

typedef enum
{
  OF_NONE,
  OF_16L16P32N,
  OF_16L16P32N_16L16P32N,
  OF_16L16L32N_16P16P32N,
  OF_24L24P16N,
  OF_24L24P16N_24L24P16N,
  OF_24L24L16N_24P24P16N,
  OF_16L16P8L8P16N_16L16P8L8P16N,
  OF_16L16L8L8L16N_16P16P8P8P16N,
  OF_NUM,
} output_format_e;

typedef enum
{
  OM_RADIO, //tryb radiowy roznica w konwersjach jest gdy multislave jest wkompilowany.
  OM_CABLE, //FullHD, lub kablowy
  OM_NUM,
  OM_NO_CHANGE,
} outputMode_e;

typedef enum
{
  RM_MULTISLAVE,
  RM_MULTISLAVE_STEREO,
  RM_SINGLESLAVE,
  RM_NUM,
  RM_NONE,
} radioMode_e;
extern radioMode_e rMode;

void fillUpSAI1(unsigned int count);
void fillUpSAI2(unsigned int count);

void formatChange(input_e input, input_format_e inputFormat, int freq);
void inputChange(input_e input, int subinput, uint8_t spkSet, outputMode_e outputMode);
void rmChange(radioMode_e radioMode, int spkSet, int wait);
void pipeSetSpkSet(int pipeNum, int spkSet, int spkNum);
int getPipeNum(int spkSet);
void printPipes(int *bufSize, char *bufPtr);
void unMute(int pipeNum);
void pipeSync(int pipeNum);

#define AUDIO_BUFFER_SAMPLES 64
#define AUDIO_BUFFER_OUT_SIZE (AUDIO_BUFFER_SAMPLES*4*sizeof(uint16_t))

#ifndef QUEUE_USB_LIMITS
#define QUEUE_USB_LIMITS 10,2,5,0
#endif

#ifndef QUEUE_SPI1_LIMITS
#define QUEUE_SPI1_LIMITS 10,2,3,0
#endif

#ifndef QUEUE_SPI2_LIMITS
#define QUEUE_SPI2_LIMITS 10,2,3,0
#endif

#ifndef QUEUE_I2S_LIMITS
#define QUEUE_I2S_LIMITS 10,3,-1,0
#endif

#ifndef QUEUE_TEST_LIMITS
#define QUEUE_TEST_LIMITS 10,3,-1,0
#endif

#ifndef QUEUE_SAI1_LIMITS
#define QUEUE_SAI1_LIMITS 15,5,-1,4
#endif

#ifndef QUEUE_SAI2_LIMITS
#define QUEUE_SAI2_LIMITS 15,5,-1,4
#endif

void __attribute__((weak))USBTrimm(unsigned int count);
void __attribute__((weak))SPI1Trimm(unsigned int count);
void __attribute__((weak))SPI2Trimm(unsigned int count);
void __attribute__((weak))I2STrimm(unsigned int count);
void __attribute__((weak))testTrimm(unsigned int count);

int getOutputType(int pipeNum);
int getOutputMode(int pipeNum);
int isMultiSlave(void);
void setMultiSlave(void);
int isHD(void);
void sendStreamInfo(int pipeNum);
void sendStreamInfoSpk(int spkSet);
extern int stereoSpkSet;
extern int twoOutputs;

extern bBuffer_t *streamCaptureBuffer;
extern input_format_e streamCaptureBufferFormat;
#define STREAM_CAPTURE_BUFFER_SIZE (((cfg.proto>>24) == 0)?(32*1024):(1024*(cfg.proto>>24)))
