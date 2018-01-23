#include <string.h>
#include <stdint.h>
#include "common.h"
#include "startup.h"
#include "project.h"
#include STMINCP()
#include STMINCS(_sai)

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <audioTypes.h>
#include "bqueue.h"
#include "convert.h"
#include "messageProcessor.h"
#include "messageProcessorMADO.h"
#include <saihwInit.h>
#include <moduleCommon.h>
#include <wm8805.h>
#include <debug.h>


enum
{
  CMD_CONVERT,
  CMD_INPUT_CHANGE,
  CMD_FORMAT_CHANGE,
  CMD_RADIO_MODE_CHANGE,
  CMD_UNMUTE,
  CMD_PIPE_SYNC,
};

static void convertMsg(void *data);
static void inputChangeMsg(void *data);
static void formatChangeMsg(void *data);
static void rmChangeMsg(void *data);
static void unmuteMsg(void *data);
static void pipeSyncMsg(void *data);
radioMode_e rMode = RM_NONE;
int stereoSpkSet = -1;
int twoOutputs = 0;

static const action_t commands[] =
{
  [CMD_CONVERT]       = convertMsg,
  [CMD_INPUT_CHANGE]  = inputChangeMsg,
  [CMD_FORMAT_CHANGE] = formatChangeMsg,
  [CMD_RADIO_MODE_CHANGE] = rmChangeMsg,
  [CMD_UNMUTE] = unmuteMsg,
  [CMD_PIPE_SYNC] = pipeSyncMsg,
};
static moduleModule_t module[1] =
{
  [0] = {
    .queueLength = 16,
    .timeout = 100,               //[ms]
    .commandsNum = sizeof(commands) / sizeof(commands[0]),
    .initCallback = NULL,
    .commandCallback = commands,
    .timeoutCallback = NULL,
    .alwaysCallback = NULL,
    .priority = 2,
    .stackSize = 2048,
    .name = "converter",
  }
};
typedef enum
{
  FREQ_CLASS_NS,
  FREQ_CLASS_DS,
  FREQ_CLASS_NUM,
} freqClass_e;

static freq_e mapFreq(int freq);
static freqClass_e mapFreqClass(int freq);
static void pipeConfigure(int pipeNum);
static int pipeRelease(int pipeNum);
#define CONV_PARAMS int pipeNum, bBuffer_t *inBuffer, bBuffer_t *outBuffer1, bBuffer_t *outBuffer2, int skipRatio
#define CONV_RET int
typedef int (*convert_t)(CONV_PARAMS);
//ds - down sampling
//ns - normal sampling

//NULL konwerter
static CONV_RET conv_NULL                                             (CONV_PARAMS);

//Wszystkie tryby 16bit
static CONV_RET conv_ns_16L16P_to_16L16P32N                           (CONV_PARAMS);
static CONV_RET conv_ns_16L16N16P16N_to_16L16P32N                     (CONV_PARAMS);
static CONV_RET conv_ds_16L16P_to_16L16L32N_16P16P32N                 (CONV_PARAMS);
static CONV_RET conv_ds_16L16N16P16N_to_16L16L32N_16P16P32N           (CONV_PARAMS);

//Tryb multislave radiowy 24 bit
static CONV_RET conv_ns_24L24P_to_16L16P32N                           (CONV_PARAMS);
static CONV_RET conv_ns_24L8N24P8N_to_16L16P32N                       (CONV_PARAMS);
static CONV_RET conv_ds_24L24P_to_16L16L32N_16P16P32N                 (CONV_PARAMS);
static CONV_RET conv_ds_24L8N24P8N_to_16L16L32N_16P16P32N             (CONV_PARAMS);
static CONV_RET conv_ns_16L8L8N16P8P8N_to_16L16P32N                   (CONV_PARAMS);
static CONV_RET conv_ds_16L8L8N16P8P8N_to_16L16L32N_16P16P32N         (CONV_PARAMS);

//Tryb kablowy lub FullHD 24bit
static CONV_RET conv_ns_24L24P_to_24L24P16N                           (CONV_PARAMS);
static CONV_RET conv_ns_24L24P_to_16L16P8L8P16N_16L16P8L8P16N         (CONV_PARAMS);
static CONV_RET conv_ns_24L8N24P8N_to_24L24P16N                       (CONV_PARAMS);
static CONV_RET conv_ns_16L8L8N16P8P8N_to_24L24P16N                   (CONV_PARAMS);
static CONV_RET conv_ns_16L8L8N16P8P8N_to_16L16P8L8P16N_16L16P8L8P16N (CONV_PARAMS);
static CONV_RET conv_ds_24L24P_to_24L24L16N_24P24P16N                 (CONV_PARAMS);
static CONV_RET conv_ds_24L8N24P8N_to_24L24L16N_24P24P16N             (CONV_PARAMS);
static CONV_RET conv_ds_24L8N24P8N_to_16L16L8L8L16N_16P16P8P8P16N     (CONV_PARAMS);
static CONV_RET conv_ds_16L8L8N16P8P8N_to_24L24L16N_24P24P16N         (CONV_PARAMS);
static CONV_RET conv_ds_16L8L8N16P8P8N_to_16L16L8L8L16N_16P16P8P8P16N (CONV_PARAMS);
static CONV_RET conv_ds_24L24P_to_16L16L8L8L16N_16P16P8P8P16N         (CONV_PARAMS);

typedef struct
{
  convert_t convert;
  uint32_t twoOutputs : 1;
} convertFormat_t;

typedef struct
{
  uint8_t         mute     :1;
  uint8_t         active   :1;
  uint8_t         subinput :4;
  uint32_t        unmuteIgnore;
  input_e         input;
  input_format_e  inputFormat;
  int             spkSets[4];
  output_e        primaryOutQueue;
  output_e        secondaryOutQueue;
  outputMode_e    outputMode;
  saiClkSource_t  primaryClkSrc;

  //Parameters taken from format configuration
  output_format_e outputFormat;
  const convertFormat_t *convertFormat;
  bQueue_t       *inQueue;
  bQueue_t       *outQueue1;
  bQueue_t       *outQueue2;
  int             inFreq;
  int             outFreq;
  int             skipRatio;
  saiClkSource_t  clkSrc;

  //Runtime parameters
  bBuffer_t      *inBuffer;
  bBuffer_t      *outBuffer1;
  bBuffer_t      *outBuffer2;

} pipe_t;

static pipe_t pipes[2] =
{
  [0] =
  {
    .mute       = 0,
    .unmuteIgnore = 0,
    .active     = 0,
    .input         = INPUT_NONE,
    .subinput      = 0,
    .inputFormat   = IF_16L16N16P16N,
    .spkSets       = {-1,-1,-1,-1},
    .outputMode    = OM_RADIO,
    .primaryClkSrc = SAI_PLL,
    .primaryOutQueue = OUTPUT_SAI2,
    .secondaryOutQueue = OUTPUT_SAI1,
    .inFreq          = 44100,
    .outFreq         = 44100,
    .skipRatio       = 1,

    .convertFormat = NULL,
    .inQueue    = NULL,
    .outQueue1  = NULL,
    .outQueue2  = NULL,
    .inBuffer   = NULL,
    .outBuffer1 = NULL,
    .outBuffer2 = NULL,
    .clkSrc     = SAI_PLL,
    .outputFormat = OF_16L16P32N,
  },
  [1] =
  {
    .mute       = 0,
    .unmuteIgnore = 0,
    .active     = 0,
    .input         = INPUT_USB2,
    .subinput      = 0,
    .inputFormat   = IF_16L16N16P16N,
    .spkSets       = {-1,-1,-1,-1},
    .outputMode    = OM_RADIO,
    .primaryClkSrc = I2S_PLL,
    .primaryOutQueue = OUTPUT_SAI1,
    .secondaryOutQueue = OUTPUT_SAI2,
    .inFreq          = 44100,
    .outFreq         = 44100,
    .skipRatio       = 1,

    .convertFormat = NULL,
    .inQueue    = NULL,
    .outQueue1  = NULL,
    .outQueue2  = NULL,
    .inBuffer   = NULL,
    .outBuffer1 = NULL,
    .outBuffer2 = NULL,
    .clkSrc     = I2S_PLL,
    .outputFormat = OF_16L16P32N,
  },
};

static pipe_t oldPipes[2] =
{
  [0 ... 1] = {0},
};

bBuffer_t *streamCaptureBuffer = NULL;
input_format_e streamCaptureBufferFormat;

#define FREQ_CLASS_ALL    FREQ_CLASS_NS ... FREQ_CLASS_DS

static const output_format_e outputFormatMap[RM_NUM][OM_NUM][IF_NUM][FREQ_CLASS_NUM] =
{
  //Initialize all to none. Later modify only used ones
  [RM_MULTISLAVE ... RM_NUM-1][OM_RADIO ... OM_NUM-1][IF_16L16P ... IF_NUM-1][FREQ_CLASS_ALL] = OF_NONE,


  [RM_MULTISLAVE] =
  {
    [OM_RADIO] =
    {
      [IF_16L16P ... IF_16L8L8N16P8P8N][FREQ_CLASS_NS ] = OF_16L16P32N,
      [IF_16L16P ... IF_16L8L8N16P8P8N][FREQ_CLASS_DS ] = OF_16L16L32N_16P16P32N,
    },

    [OM_CABLE] =
    {
      [IF_16L16P ... IF_16L16N16P16N  ][FREQ_CLASS_NS ] = OF_16L16P32N_16L16P32N,
      [IF_16L16P ... IF_16L16N16P16N  ][FREQ_CLASS_DS ] = OF_16L16L32N_16P16P32N,

      [IF_24L24P ... IF_16L8L8N16P8P8N][FREQ_CLASS_NS ] = OF_24L24P16N_24L24P16N,
      [IF_24L24P ... IF_16L8L8N16P8P8N][FREQ_CLASS_DS ] = OF_24L24L16N_24P24P16N,
    },
  },

  [RM_MULTISLAVE_STEREO] =
  {
    [OM_RADIO] =
    {
      [IF_16L16P ... IF_16L8L8N16P8P8N][FREQ_CLASS_NS ] = OF_16L16P32N_16L16P32N,
      [IF_16L16P ... IF_16L8L8N16P8P8N][FREQ_CLASS_DS ] = OF_16L16L32N_16P16P32N,
    },

    [OM_CABLE] =
    {
      [IF_16L16P ... IF_16L16N16P16N  ][FREQ_CLASS_NS ] = OF_16L16P32N_16L16P32N,
      [IF_16L16P ... IF_16L16N16P16N  ][FREQ_CLASS_DS ] = OF_16L16L32N_16P16P32N,

      [IF_24L24P ... IF_16L8L8N16P8P8N][FREQ_CLASS_NS ] = OF_24L24P16N_24L24P16N,
      [IF_24L24P ... IF_16L8L8N16P8P8N][FREQ_CLASS_DS ] = OF_24L24L16N_24P24P16N,
    },
  },

  [RM_SINGLESLAVE] =
  {
    [OM_RADIO] =
    {
      [IF_16L16P ... IF_16L16N16P16N  ][FREQ_CLASS_NS ] = OF_16L16P32N_16L16P32N,
      [IF_16L16P ... IF_16L16N16P16N  ][FREQ_CLASS_DS ] = OF_16L16L32N_16P16P32N,

      [IF_24L24P ... IF_16L8L8N16P8P8N][FREQ_CLASS_NS ] = OF_16L16P8L8P16N_16L16P8L8P16N,
      [IF_24L24P ... IF_16L8L8N16P8P8N][FREQ_CLASS_DS ] = OF_16L16L8L8L16N_16P16P8P8P16N,
    },

    [OM_CABLE] =
    {
      [IF_16L16P ... IF_16L16N16P16N  ][FREQ_CLASS_NS ] = OF_16L16P32N_16L16P32N,
      [IF_16L16P ... IF_16L16N16P16N  ][FREQ_CLASS_DS ] = OF_16L16L32N_16P16P32N,

      [IF_24L24P ... IF_16L8L8N16P8P8N][FREQ_CLASS_NS ] = OF_24L24P16N_24L24P16N,
      [IF_24L24P ... IF_16L8L8N16P8P8N][FREQ_CLASS_DS ] = OF_24L24L16N_24P24P16N,
    },
  },
};


#define CFORMAT(arg_function, arg_two) {.convert = arg_function, .twoOutputs = arg_two,}
#define OPTIM "-O0"
const convertFormat_t convertTable[IF_NUM][OF_NUM] =
{
  [0 ... IF_NUM - 1] [0 ... OF_NUM - 1]               = CFORMAT(conv_NULL, 1),
  [0 ... IF_NUM - 1] [OF_16L16P32N]                   = CFORMAT(conv_NULL, 0),
  [0 ... IF_NUM - 1] [OF_24L24P16N]                   = CFORMAT(conv_NULL, 0),
  [IF_16L16P]        [OF_16L16P32N]                   = CFORMAT(conv_ns_16L16P_to_16L16P32N, 0),
  [IF_16L16P]        [OF_16L16P32N_16L16P32N]         = CFORMAT(conv_ns_16L16P_to_16L16P32N, 1),
  [IF_16L16P]        [OF_16L16L32N_16P16P32N]         = CFORMAT(conv_ds_16L16P_to_16L16L32N_16P16P32N, 1),
  [IF_16L16N16P16N]  [OF_16L16P32N]                   = CFORMAT(conv_ns_16L16N16P16N_to_16L16P32N, 0),
  [IF_16L16N16P16N]  [OF_16L16P32N_16L16P32N]         = CFORMAT(conv_ns_16L16N16P16N_to_16L16P32N, 1),
  [IF_16L16N16P16N]  [OF_16L16L32N_16P16P32N]         = CFORMAT(conv_ds_16L16N16P16N_to_16L16L32N_16P16P32N, 1),
  [IF_24L24P]        [OF_16L16P32N]                   = CFORMAT(conv_ns_24L24P_to_16L16P32N, 0),
  [IF_24L24P]        [OF_16L16P32N_16L16P32N]         = CFORMAT(conv_ns_24L24P_to_16L16P32N, 1),
  [IF_24L24P]        [OF_16L16L32N_16P16P32N]         = CFORMAT(conv_ds_24L24P_to_16L16L32N_16P16P32N, 1),
  [IF_24L24P]        [OF_24L24P16N]                   = CFORMAT(conv_ns_24L24P_to_24L24P16N, 0),
  [IF_24L24P]        [OF_24L24P16N_24L24P16N]         = CFORMAT(conv_ns_24L24P_to_24L24P16N, 1),
  [IF_24L24P]        [OF_24L24L16N_24P24P16N]         = CFORMAT(conv_ds_24L24P_to_24L24L16N_24P24P16N, 1),
  [IF_24L24P]        [OF_16L16P8L8P16N_16L16P8L8P16N] = CFORMAT(conv_ns_24L24P_to_16L16P8L8P16N_16L16P8L8P16N, 1),
  [IF_24L24P]        [OF_16L16L8L8L16N_16P16P8P8P16N] = CFORMAT(conv_ds_24L24P_to_16L16L8L8L16N_16P16P8P8P16N, 1),
  [IF_24L8N24P8N]    [OF_16L16P32N]                   = CFORMAT(conv_ns_24L8N24P8N_to_16L16P32N, 0),
  [IF_24L8N24P8N]    [OF_16L16P32N_16L16P32N]         = CFORMAT(conv_ns_24L8N24P8N_to_16L16P32N, 1),
  [IF_24L8N24P8N]    [OF_16L16L32N_16P16P32N]         = CFORMAT(conv_ds_24L8N24P8N_to_16L16L32N_16P16P32N, 1),
  [IF_24L8N24P8N]    [OF_24L24P16N]                   = CFORMAT(conv_ns_24L8N24P8N_to_24L24P16N, 0),
  [IF_24L8N24P8N]    [OF_24L24L16N_24P24P16N]         = CFORMAT(conv_ds_24L8N24P8N_to_24L24L16N_24P24P16N, 1),
  [IF_24L8N24P8N]    [OF_16L16L8L8L16N_16P16P8P8P16N] = CFORMAT(conv_ds_24L8N24P8N_to_16L16L8L8L16N_16P16P8P8P16N, 1),
  [IF_16L8L8N16P8P8N][OF_16L16P32N]                   = CFORMAT(conv_ns_16L8L8N16P8P8N_to_16L16P32N, 0),
  [IF_16L8L8N16P8P8N][OF_16L16P32N_16L16P32N]         = CFORMAT(conv_ns_16L8L8N16P8P8N_to_16L16P32N, 1),
  [IF_16L8L8N16P8P8N][OF_16L16L32N_16P16P32N]         = CFORMAT(conv_ds_16L8L8N16P8P8N_to_16L16L32N_16P16P32N, 1),
  [IF_16L8L8N16P8P8N][OF_24L24P16N]                   = CFORMAT(conv_ns_16L8L8N16P8P8N_to_24L24P16N, 0),
  [IF_16L8L8N16P8P8N][OF_24L24P16N_24L24P16N]         = CFORMAT(conv_ns_16L8L8N16P8P8N_to_24L24P16N, 1),
  [IF_16L8L8N16P8P8N][OF_24L24L16N_24P24P16N]         = CFORMAT(conv_ds_16L8L8N16P8P8N_to_24L24L16N_24P24P16N, 1),
  [IF_16L8L8N16P8P8N][OF_16L16P8L8P16N_16L16P8L8P16N] = CFORMAT(conv_ns_16L8L8N16P8P8N_to_16L16P8L8P16N_16L16P8L8P16N, 1),
  [IF_16L8L8N16P8P8N][OF_16L16L8L8L16N_16P16P8P8P16N] = CFORMAT(conv_ds_16L8L8N16P8P8N_to_16L16L8L8L16N_16P16P8P8P16N, 1),
};

static const int freqDiv[OM_NUM][FREQ_NUM] =
{
  [OM_RADIO] =
  {
    [FREQ_44100 ... FREQ_48000] = 1,
    [FREQ_88200 ... FREQ_96000] = 2,
    [FREQ_176400 ... FREQ_192000] = 4,
  },
  [OM_CABLE] =
  {
    [FREQ_44100 ... FREQ_48000] = 1,
    [FREQ_88200 ... FREQ_192000] = 2,
  },
};

static const int skipRatio[OM_NUM][FREQ_NUM] =
{
  [OM_RADIO] =
  {
    [FREQ_44100 ... FREQ_48000] = 1,
    [FREQ_88200 ... FREQ_96000] = 1,
    [FREQ_176400 ... FREQ_192000] = 2,
  },
  [OM_CABLE] =
  {
    [FREQ_44100 ... FREQ_48000] = 1,
    [FREQ_88200 ... FREQ_192000] = 1,
  },
};

static const char *clkSrcStr[] =
{
  [SAI_PLL] = "SAI",
  [I2S_PLL] = "I2S",
  [EXTERNAL] = "EXT",
};

static const char *inQ[INPUT_NUM] =
{
  [INPUT_NONE] = "None",
  [INPUT_USB1]  = "USB",
  [INPUT_USB2]  = "USB",
  [INPUT_SPI1] = "SPI1",
  [INPUT_SPI2] = "SPI2",
  [INPUT_SPDIF1]  = "WM1",
  [INPUT_SPDIF2]  = "WM2",
  [INPUT_TEST] = "TEST",
};

/* static const char *outQ[OUTPUT_NUM] = */
/* { */
/*   [OUTPUT_NONE] = "None", */
/*   [OUTPUT_SAI1] = "SAI1", */
/*   [OUTPUT_SAI2] = "SAI2", */
/* }; */

static const char *inFormat[IF_NUM] =
{
  [IF_16L16P]         = "16L16P",
  [IF_24L24P]         = "24L24P",
  [IF_24L8N24P8N]     = "24L8N24P8N",
  [IF_16L8L8N16P8P8N] = "16L8L8N16P8P8N",
  [IF_16L16N16P16N]   = "16L16N16P16N",

};

static const char *outMode[OM_NUM] =
{
  [OM_RADIO] = "Radio",
  [OM_CABLE] = "Cable",
};

static const char *outFormat[OF_NUM] =
{
  [OF_NONE]                      = "NONE",
  [OF_16L16P32N]                 = "16L16P32N",
  [OF_16L16P32N_16L16P32N]       = "16L16P32N_16L16P32N",
  [OF_16L16L32N_16P16P32N]       = "16L16L32N_16P16P32N",
  [OF_24L24P16N]                 = "24L24P16N",
  [OF_24L24P16N_24L24P16N]       = "24L24P16N_24L24P16N",
  [OF_24L24L16N_24P24P16N]       = "24L24L16N_24P24P16N",
  [OF_16L16P8L8P16N_16L16P8L8P16N] = "16L16P8L8P16N_16L16P8L8P16N",
  [OF_16L16L8L8L16N_16P16P8P8P16N] = "16L16L8L8L16N_16P16P8P8P16N",
};

int isHD(void)
{
  if(pipes[0].active && (pipes[0].inFreq >= 88200 || pipes[0].inputFormat >= IF_24L24P)) return 1;
  if(pipes[1].active && (pipes[1].inFreq >= 88200 || pipes[1].inputFormat >= IF_24L24P)) return 1;

  return 0;
}

static void convert(int pipeNum);

static int outQueuePipeAlloc(bQueue_t *queue)
{
  if (pipes[0].outQueue1 == queue || pipes[0].outQueue2 == queue)
    return 0;
  if (pipes[1].outQueue1 == queue || pipes[1].outQueue2 == queue)
    return 1;
  return -1;
}

void fillUpSAI1(unsigned int count)
{
  int pipe = outQueuePipeAlloc(&outQueueSAI1);
  if(pipe >= 0)
    {
      if (moduleSendCommand(&module[0], CMD_CONVERT, (void*)pipe) < 0)
        {
          //Sending failed
        }
    }
}
void fillUpSAI2(unsigned int count)
{
  int pipe = outQueuePipeAlloc(&outQueueSAI2);
  if(pipe >= 0)
    {
      if (moduleSendCommand(&module[0], CMD_CONVERT, (void*)pipe) < 0)
        {
          //Sending failed
        }
    }
}

bQueue_t inQueueUSB1   = BQUEUE_STATIC_INIT(0, 0, 0, 0, USBTrimm);
bQueue_t inQueueUSB2   = BQUEUE_STATIC_INIT(0, 0, 0, 0, USBTrimm);
bQueue_t inQueueSPI1  = BQUEUE_STATIC_INIT(0, 0, 0, 0, SPI1Trimm);
bQueue_t inQueueSPI2  = BQUEUE_STATIC_INIT(0, 0, 0, 0, SPI2Trimm);
bQueue_t inQueueI2S   = BQUEUE_STATIC_INIT(0, 0, 0, 0, I2STrimm);
bQueue_t inQueueTest  = BQUEUE_STATIC_INIT(0, 0, 0, 0, testTrimm);
bQueue_t outQueueSAI1 = BQUEUE_STATIC_INIT(0, 0, 0, 0, fillUpSAI1);
bQueue_t outQueueSAI2 = BQUEUE_STATIC_INIT(0, 0, 0, 0, fillUpSAI2);

static bQueue_t *inQueues[INPUT_NUM] =
{
  [INPUT_NONE] = NULL,
  [INPUT_USB1]  = &inQueueUSB1,
  [INPUT_USB2]  = &inQueueUSB2,
  [INPUT_SPI1] = &inQueueSPI1,
  [INPUT_SPI2] = &inQueueSPI2,
  [INPUT_SPDIF1]  = NULL,
  [INPUT_SPDIF2]  = &inQueueI2S,
  [INPUT_TEST] = &inQueueTest,
};
static bQueue_t *outQueues[OUTPUT_NUM] =
{
  [OUTPUT_NONE]  = NULL,
  [OUTPUT_SAI1]  = &outQueueSAI1,
  [OUTPUT_SAI2]  = &outQueueSAI2,
};

static int cInFreq[INPUT_NUM] =
{
  [INPUT_NONE] = 0,
  [INPUT_USB1]  = 44100,
  [INPUT_USB2]  = 44100,
  [INPUT_SPI1] = 44100,
  [INPUT_SPI2] = 44100,
  [INPUT_SPDIF1]  = 44100,
  [INPUT_SPDIF2]  = 44100,
  [INPUT_TEST] = 48000,
};
static uint8_t cInValid[INPUT_NUM] =
{
  [INPUT_NONE] = 0,
  [INPUT_USB1]  = 1,
  [INPUT_USB2]  = 1,
  [INPUT_SPI1] = 1,
  [INPUT_SPI2] = 1,
  [INPUT_SPDIF1]  = 1,
  [INPUT_SPDIF2]  = 1,
  [INPUT_TEST] = 1,
};

static input_format_e cInputFormat[INPUT_NUM] =
{
  [INPUT_NONE] = IF_16L16P,
  [INPUT_USB1]  = IF_16L16P,
  [INPUT_USB2]  = IF_16L16P,
  [INPUT_SPI1] = IF_16L16P,
  [INPUT_SPI2] = IF_16L16P,
  [INPUT_SPDIF1]  = IF_16L16N16P16N,
  [INPUT_SPDIF2]  = IF_16L16N16P16N,
  [INPUT_TEST] = IF_16L16P,
};

static freq_e mapFreq(int freq)
{
  switch (freq)
    {
    case 44100:
      return FREQ_44100;
      break;
    case 48000:
      return FREQ_48000;
      break;
    case 88200:
      return FREQ_88200;
      break;
    case 96000:
      return FREQ_96000;
      break;
    case 176400:
      return FREQ_176400;
      break;
    case 192000:
      return FREQ_192000;
      break;
    }
  return FREQ_NONE;
}

static freqClass_e mapFreqClass(int freq)
{
  switch (freq)
    {
    case 44100:
    case 48000:
      return FREQ_CLASS_NS;
    case 88200:
    case 96000:
    case 176400:
    case 192000:
      return FREQ_CLASS_DS;
    }
  return FREQ_CLASS_NS;
}


static const uint8_t audioOutputType[FREQ_NUM][OF_NUM] =
{
  //None initially
  [FREQ_NONE ... FREQ_192000][OF_NONE ... OF_24L24L16N_24P24P16N] = 0,
  [FREQ_44100][OF_16L16P32N]            = 0x01,
  [FREQ_44100][OF_16L16P32N_16L16P32N]  = 0x01,
  [FREQ_44100][OF_24L24P16N]            = 0x01,
  [FREQ_44100][OF_24L24P16N_24L24P16N]  = 0x01,
  [FREQ_48000][OF_16L16P32N]            = 0x02,
  [FREQ_48000][OF_16L16P32N_16L16P32N]  = 0x02,
  [FREQ_48000][OF_24L24P16N]            = 0x02,
  [FREQ_48000][OF_24L24P16N_24L24P16N]  = 0x02,
  [FREQ_88200][OF_16L16L32N_16P16P32N]  = 0x0A,
  [FREQ_96000][OF_16L16L32N_16P16P32N]  = 0x0B,
  [FREQ_88200][OF_24L24L16N_24P24P16N]  = 0x0C,
  [FREQ_96000][OF_24L24L16N_24P24P16N]  = 0x0D,
  [FREQ_88200][OF_16L16L8L8L16N_16P16P8P8P16N]  = 0x0C,
  [FREQ_96000][OF_16L16L8L8L16N_16P16P8P8P16N]  = 0x0D,
  [FREQ_176400][OF_16L16L32N_16P16P32N] = 0x11,
  [FREQ_192000][OF_16L16L32N_16P16P32N] = 0x12,
  [FREQ_176400][OF_24L24L16N_24P24P16N] = 0x13,
  [FREQ_192000][OF_24L24L16N_24P24P16N] = 0x14,
  [FREQ_176400][OF_16L16L8L8L16N_16P16P8P8P16N] = 0x13,
  [FREQ_192000][OF_16L16L8L8L16N_16P16P8P8P16N] = 0x14,
};

int getOutputType(int pipeNum)
{
  pipe_t *pipe = &pipes[pipeNum];
  if(pipe)
    return audioOutputType[mapFreq(pipe->inFreq)][pipe->outputFormat];
  return 0;
}

int getOutputMode(int pipeNum)
{
  pipe_t *pipe = &pipes[pipeNum];
  if(pipe)
    return (pipe->outputMode == OM_RADIO)?1:2;
  return 0;
}

static void sendStreamInfoSpkPipe(int pipeNum, int spkSet)
{
  pipe_t *pipe = &pipes[pipeNum];
  freq_e fr = mapFreq(pipe->inFreq);

  if (fr != FREQ_NONE)
    {
      //Send freq change to SMOK
      bBuffer_t *msg = bAlloc(MSG_LEN);
      msg->size = sizeof(msg_92_t);
      msg_92_t *data = (void *)msg->data;
      memset(data, 0, MSG_LEN);
      data->kod = 0x92;
      data->kondPom = spkSet;
      data->signalType = audioOutputType[mapFreq(pipe->inFreq)][pipe->outputFormat];
      data->outputType = (pipe->outputMode == OM_RADIO)?1:2;
      msgGenSend(msg);
    }
}

void sendStreamInfo(int pipeNum)
{
  pipe_t *pipe = &pipes[pipeNum];
  int spkNum;
  for(spkNum = 0; spkNum < 4; spkNum++)
    if(pipe->spkSets[spkNum] >= 0)
      sendStreamInfoSpkPipe(pipeNum, pipe->spkSets[spkNum]);
}

void sendStreamInfoSpk(int spkSet)
{
  int pipeNum;
  for(pipeNum = 0; pipeNum < 2; pipeNum++)
    {
      pipe_t *pipe = &pipes[pipeNum];
      int spkNum;
      for(spkNum = 0; spkNum < 4; spkNum++)
        if(pipe->spkSets[spkNum] == spkSet)
          sendStreamInfoSpkPipe(pipeNum, spkSet);
    }
}

static void streamDisable(int pipeNum)
{
  if(pipeNum < 0) return;
  int muteFreq = pipeRelease(pipeNum);
  pipe_t *pipe = &pipes[pipeNum];
  pipe->clkSrc = pipe->primaryClkSrc;
  pipe->active = 0;
  int spkNum;
  for(spkNum = 0; spkNum < 4; spkNum++)
    if(pipe->spkSets[spkNum] >= 0)
      {
        //Send freq change to SMOK
        bBuffer_t *msg = bAlloc(MSG_LEN);
        msg->size = sizeof(msg_92_t);
        msg_92_t *data = (void *)msg->data;
        memset(data, 0, MSG_LEN);
        data->kod = 0x92;
        data->kondPom = pipe->spkSets[spkNum];
        //Send any format with a proper mute frequency (the same as transmitted thtough radio)
        data->signalType = audioOutputType[mapFreq(muteFreq)][OF_16L16P32N];
        data->outputType = (pipe->outputMode == OM_RADIO)?1:2;
        msgGenSend(msg);
      }
}

void pipeSetSpkSet(int pipeNum, int spkSet, int spkNum)
{
  if(spkNum == -1)
    {
      for(spkNum = 0; spkNum < 4; spkNum++)
        {
          if(pipes[0].spkSets[spkNum] == spkSet)break;
          if(pipes[1].spkSets[spkNum] == spkSet)break;
        }
      //exit if not found
      if(spkNum > 3) return;
    }
  pipes[0].spkSets[spkNum] = -1;
  pipes[1].spkSets[spkNum] = -1;
  if(pipeNum >= 0)
    pipes[pipeNum].spkSets[spkNum] = spkSet;
}

#define PIPE_SPK_SET 0
#define PIPE_INPUT   1
int getPipeNum(int spkSet)
{
  int i;
  for(i = 0; i < sizeof(pipes)/sizeof(pipes[0]); i++)
    {
      int spkNum;
      for(spkNum = 0; spkNum < 4; spkNum++)
        if(pipes[i].spkSets[spkNum] == spkSet) return i;
    }
  return -1;;
}

static int getPipe(uint8_t searchFilter, uint32_t value)
{
  int i;
  for(i = 0; i < sizeof(pipes)/sizeof(pipes[0]); i++)
    {
      switch(searchFilter)
        {
        case PIPE_SPK_SET:
        {
          int spkNum;
          for(spkNum = 0; spkNum < 4; spkNum++)
            if(pipes[i].spkSets[spkNum] == value) return i;
        }
        break;
        case PIPE_INPUT:
          if(pipes[i].input == value)         return i;
          break;
        }
    }
  return -1;
}

typedef struct
{
  input_e input;
  int     subInput;
  uint8_t spkSet;
  outputMode_e outputMode;
} inputChange_t;
void inputChange(input_e input, int subinput, uint8_t spkSet, outputMode_e outputMode)
{
  //Allocate resources for data transfer
  inputChange_t *message = dAlloc(sizeof(inputChange_t));

  if (message == NULL)
    return;

  //Fill message
  message->input = input;
  message->subInput = subinput;
  message->spkSet = spkSet;
  message->outputMode = outputMode;

  //Send the message to communicator
  if (moduleSendCommand(module, CMD_INPUT_CHANGE, message) < 0)
    {
      //Sending failed
      //Free allocated resources
      dFree(message);
    }
}
static void inputChangeMsg(void *data)
{
  inputChange_t *message = data;
  int pipeNum = getPipe(PIPE_SPK_SET, message->spkSet);
  if(pipeNum < 0)
    {
      dprintf(LL_DEBUG, "Pipe for spkSet %x not found.\n", message->spkSet);
      dFree(message);
      return;
    }

  if(message->outputMode != OM_NO_CHANGE)
    {
      pipes[0].outputMode = message->outputMode;
      pipes[1].outputMode = message->outputMode;
      streamDisable(0);
      streamDisable(1);
    }

  pipe_t *pipe = &pipes[pipeNum];

  //Reconfigure pipe
  //  if(pipe->input != message->input)
  {
    pipe->input = message->input;
    pipe->subinput = message->subInput;
    pipe->inputFormat = cInputFormat[message->input];
    pipe->inFreq = cInFreq[message->input];
    pipeConfigure(pipeNum);
  }

  dFree(message);
}

typedef struct
{
  radioMode_e radioMode;
  int spkSet;
} rmChange_t;
void rmChange(radioMode_e radioMode, int spkSet, int wait)
{
  //Allocate resources for data transfer
  rmChange_t *message = dAlloc(sizeof(rmChange_t));

  if (message == NULL)
    return;

  //Fill message
  message->radioMode = radioMode;
  message->spkSet = spkSet;

  //Send the message to communicator
  if (moduleSendCommand(module, CMD_RADIO_MODE_CHANGE, message) < 0)
    {
      //Sending failed
      //Free allocated resources
      dFree(message);
    }
  else
    while(wait && radioMode != rMode) mdelay(1);

}
static void rmChangeMsg(void *data)
{
  stereoSpkSet = -1;
  twoOutputs = 0;
  rmChange_t *message = data;
  switch(message->radioMode)
    {
    default:
      break;
    case RM_MULTISLAVE_STEREO:
    case RM_SINGLESLAVE:
      twoOutputs = 1;
      stereoSpkSet = message->spkSet;
    case RM_MULTISLAVE:
      if(message->radioMode != rMode)
        {
          rMode = message->radioMode;
          pipeConfigure(0);
          pipeConfigure(1);
        }
      break;
    }

  dFree(message);
}

typedef struct
{
  int pipeNum;
} unmute_t;

void unMute(int pipeNum)
{
  //Allocate resources for data transfer
  unmute_t *message = dAlloc(sizeof(unmute_t));

  if (message == NULL)
    return;

  //Fill message
  message->pipeNum = pipeNum;

  //Send the message to communicator
  if (moduleSendCommand(module, CMD_UNMUTE, message) < 0)
    {
      //Sending failed
      //Free allocated resources
      dFree(message);
    }
}

static void unmuteMsg(void *data)
{
  unmute_t *message = data;
  pipes[message->pipeNum].mute = 0;
  pipes[message->pipeNum].unmuteIgnore = cfg.unmuteIgnore + xTaskGetTickCount();
  dFree(message);
}

typedef struct
{
  input_e input;
  input_format_e inputFormat;
  int freq;
} formatChange_t;

void formatChange(input_e input, input_format_e inputFormat, int freq)
{
  //Allocate resources for data transfer
  formatChange_t *message = dAlloc(sizeof(formatChange_t));

  if (message == NULL)
    return;

  //Fill message
  message->input = input;
  message->inputFormat = inputFormat;
  message->freq = freq;

  //Send the message to communicator
  if (moduleSendCommand(module, CMD_FORMAT_CHANGE, message) < 0)
    {
      //Sending failed
      //Free allocated resources
      dFree(message);
    }
}
static void formatChangeMsg(void *data)
{
  formatChange_t *message = data;
  uint8_t inValidPrevious = cInValid[message->input];
  if(message->freq != 0)
    cInValid[message->input] = 1;
  else
    cInValid[message->input] = 0;

  int pipeNum = getPipe(PIPE_INPUT, message->input);
  dprintf(LL_INFO, "Input format %s@%d from %s \n", inFormat[message->inputFormat], message->freq, inQ[message->input]);
  if(pipeNum < 0)
    {
      //Input not selected.
      dprintf(LL_INFO, "Format change: Input %s not active.\n", inQ[message->input]);
      if(mapFreq(message->freq) != FREQ_NONE)
        {
          cInputFormat[message->input] = message->inputFormat;
          cInFreq[message->input]      = message->freq;
        }
      dFree(message);
      return;
    }
  pipe_t *pipe = &pipes[pipeNum];

  //Reconfigure pipe
  if(cInputFormat[message->input] != message->inputFormat ||
      cInFreq[message->input] != message->freq ||
      inValidPrevious != cInValid[message->input] )
    {
      pipe->inputFormat = cInputFormat[message->input] = message->inputFormat;
      if(mapFreq(message->freq) != FREQ_NONE)
        {
          pipe->inFreq = cInFreq[message->input] = message->freq;

          pipeConfigure(pipeNum);
        }
      else
        {
          streamDisable(pipeNum);
        }
    }
  dFree(message);
}


typedef struct
{
  int     pipeNum;
} pipeSync_t;
void pipeSync(int pipeNum)
{
  //Allocate resources for data transfer
  pipeSync_t *message = dAlloc(sizeof(pipeSync_t));

  if (message == NULL)
    return;

  //Fill message
  message->pipeNum = pipeNum;

  //Send the message to communicator
  if (moduleSendCommand(module, CMD_PIPE_SYNC, message) < 0)
    {
      //Sending failed
      //Free allocated resources
      dFree(message);
    }
}

static void pipeSyncMsg(void *data)
{
  pipeSync_t *message = data;
  pipeConfigure(message->pipeNum);
  dFree(message);
}

static int pipeRelease(int pipeNum)
{
  if(pipeNum < 0) return 44100;

  if(pipeNum == 1)
    selectorSet(SELECTOR_SAI);

  pipe_t *pipe = &pipes[pipeNum];
#ifdef USE_MUTE
  pipe->mute = 1;
#endif
  int mutefreq = 44100;
  switch (pipe->inFreq)
    {
    case 48000:
    case 96000:
    case 192000:
      mutefreq = 48000;
      break;
    }
  int mask = 0;
  if(pipe->outQueue1) mask |= (1<<(pipe->primaryOutQueue == OUTPUT_SAI2));
  if(pipe->outQueue2) mask |= (1<<(pipe->secondaryOutQueue == OUTPUT_SAI2));
  saiReconfigure(mask, pipe->primaryClkSrc, mutefreq, 1);
  pipe->active = 0;
  pipe->convertFormat = NULL;
  pipe->inQueue = NULL;
  pipe->outQueue1 = NULL;
  pipe->outQueue2 = NULL;

  //Release old resources
  if (pipe->inBuffer)
    {
      bFree(pipe->inBuffer);
      pipe->inBuffer = NULL;
    }
  if (pipe->outBuffer1)
    {
      bFree(pipe->outBuffer1);
      pipe->outBuffer1 = NULL;
    }
  if (pipe->outBuffer2)
    {
      bFree(pipe->outBuffer2);
      pipe->outBuffer2 = NULL;
    }
  return mutefreq;
}

#define PIPE_DISABLE_RETURN(arg_pipeNum){streamDisable(arg_pipeNum);return;}

void i2sResync(void);

static void pipeConfigure(int pipeNum)
{
  if(pipeNum < 0) return;
  pipe_t *pipe = &pipes[pipeNum];
  massert(pipe->inputFormat < IF_NUM);
  massert(pipe->outputMode < OM_NUM);
  massert(pipe->input < INPUT_NUM);

  if(pipe->input == INPUT_SPDIF2)
    {
      i2sSelectSource(0, pipe->subinput);
      i2sResync();
    }
  if(pipe->input == INPUT_SPDIF1)
    {
      i2sSelectSource(1, pipe->subinput);
    }

  if(pipeNum == 1)
    {
      //Pipe 1 not available in cable mode
      if(pipe->outputMode == OM_CABLE)
        {
          PIPE_DISABLE_RETURN(pipeNum);
        }

    }

  if(pipe->input == INPUT_NONE)
    PIPE_DISABLE_RETURN(pipeNum);
  if(mapFreq(pipe->inFreq) == FREQ_NONE)
    PIPE_DISABLE_RETURN(pipeNum);
  if(!cInValid[pipe->input])
    PIPE_DISABLE_RETURN(pipeNum);
  if(rMode == RM_NONE)
    PIPE_DISABLE_RETURN(pipeNum);
  massert(mapFreq(pipe->inFreq) != FREQ_NONE);
  output_format_e outputFormat = outputFormatMap[rMode][pipe->outputMode][pipe->inputFormat][mapFreqClass(pipe->inFreq)];
  if(outputFormat == OF_NONE)
    {
      dprintf(LL_WARNING, "Output format not found for input format:%s outputMode:%s input:%s freq:%d \n", inFormat[pipe->inputFormat], outMode[pipe->outputMode], inQ[pipe->input] , pipe->inFreq);
      PIPE_DISABLE_RETURN(pipeNum);
    }

  dprintf(LL_WARNING, "searching for convertion routine %s %s \n", inFormat[pipe->inputFormat], outFormat[outputFormat]);
  const convertFormat_t *convertFormat = &convertTable[pipe->inputFormat][outputFormat];
  if(!convertFormat->convert)
    {
      dprintf(LL_WARNING, "Convertion routine not found for %s %s \n", inFormat[pipe->inputFormat], outFormat[outputFormat]);
      PIPE_DISABLE_RETURN(pipeNum);
    }
  if(convertFormat->convert == conv_NULL)
    {
      dprintf(LL_WARNING, "NULL Convertion routine for %s %s \n", inFormat[pipe->inputFormat], outFormat[outputFormat]);
    }

  //Check if outputs are free
  if(outQueuePipeAlloc(outQueues[pipe->primaryOutQueue]) >= 0 && pipe->outQueue1 != outQueues[pipe->primaryOutQueue])
    {
      dprintf(LL_WARNING, "Primary output queue is not available.\n\n", inFormat[pipe->inputFormat], outFormat[outputFormat]);
      PIPE_DISABLE_RETURN(pipeNum);
    }
  if(convertFormat->twoOutputs)
    {
      if(outQueuePipeAlloc(outQueues[pipe->secondaryOutQueue]) >= 0 && pipe->outQueue2 != outQueues[pipe->secondaryOutQueue])
        {
          dprintf(LL_INFO, "Secondary output queue is not available.\n\n", inFormat[pipe->inputFormat], outFormat[outputFormat]);
          PIPE_DISABLE_RETURN(pipeNum);
        }

    }

  //Return if pipe not changed
  if(!memcmp((void*)&oldPipes[pipeNum], (void*)&pipes[pipeNum], offsetof(pipe_t, inBuffer)))
    {
      return;
    }

  pipeRelease(pipeNum);

  twoOutputs = convertFormat->twoOutputs;
  int div = freqDiv[pipe->outputMode][mapFreq(pipe->inFreq)];
  pipe->outFreq = pipe->inFreq/div;
  pipe->skipRatio = skipRatio[pipe->outputMode][mapFreq(pipe->inFreq)];
  if(pipe->input != INPUT_SPDIF1)
    {
      pipe->convertFormat = convertFormat;
      pipe->inQueue   = inQueues[pipe->input];
      pipe->outQueue1 = outQueues[pipe->primaryOutQueue];
      if(convertFormat->twoOutputs || pipe->outputMode == OM_CABLE)
        pipe->outQueue2 = outQueues[pipe->secondaryOutQueue];
      else
        pipe->outQueue2 = NULL;

      pipe->clkSrc = pipe->primaryClkSrc;
      if(pipe->input == INPUT_SPDIF2)
        pipe->clkSrc = EXTERNAL;

      //Reconfigure SAI
      int mask = 0;
      if(pipe->outQueue1) mask |= (1<<(pipe->primaryOutQueue == OUTPUT_SAI2));
      if(pipe->outQueue2) mask |= (1<<(pipe->secondaryOutQueue == OUTPUT_SAI2));
      saiReconfigure(mask, pipe->clkSrc, pipe->inFreq, div);
    }
  else
    {
      pipe->clkSrc = EXTERNAL;
      selectorSet(SELECTOR_WM8805);
    }

  pipe->outputFormat = outputFormat;

  switch(outputFormat)
    {
    case OF_16L16P32N:
    case OF_16L16P32N_16L16P32N:
    case OF_16L16L32N_16P16P32N:
    case OF_16L16P8L8P16N_16L16P8L8P16N:
    case OF_16L16L8L8L16N_16P16P8P8P16N:
      WM8804setResolution(WM8804_1, 16);
      WM8804setResolution(WM8804_2, 16);
      break;
    case OF_24L24P16N:
    case OF_24L24P16N_24L24P16N:
    case OF_24L24L16N_24P24P16N:
      WM8804setResolution(WM8804_1, 24);
      WM8804setResolution(WM8804_2, 24);
      break;
    case OF_NONE:
    case OF_NUM:
      break;
    }

  dprintf(LL_INFO, "P%d audio inQ %s.%d inFormat %s@%dHz outFormat %s@%dHz \n",
          pipeNum, inQ[pipe->input], pipe->subinput,
          inFormat[pipe->inputFormat], pipe->inFreq, outFormat[pipe->outputFormat], pipe->outFreq);

  sendStreamInfo(pipeNum);
  pipe->active = 1;
  oldPipes[pipeNum] = pipes[pipeNum];
}

#if defined(CONVERT_DBG)
static void printDbgBuf(int pipeNum, int *bufSize, char *bufPtr);
#else
#define printDbgBuf(...)
#endif
void printPipes(int *bufSize, char *bufPtr)
{
  int i;
  for(i = 0; i < 2; i++)
    {
      DPRINTF("P%d(%s) |", i, (pipes[i].active)?((pipes[i].mute || pipes[i].unmuteIgnore > xTaskGetTickCount())?"M":"A"):"I");
      int spkNum;
      for(spkNum = 0; spkNum < 4; spkNum++)
        if(pipes[i].spkSets[spkNum] >=0)
          {
            DPRINTF("%02x|",pipes[i].spkSets[spkNum]);
          }
        else
          {
            DPRINTF("  |");
          }


      DPRINTF(" inQ:%s.%d if:%s@%dHz of:%s@%dHz mode:%s clkSrc:%s"CLEAR_LINE"\n",
              inQ[pipes[i].input], pipes[i].subinput, inFormat[pipes[i].inputFormat],
              pipes[i].inFreq, outFormat[pipes[i].outputFormat], pipes[i].outFreq,
              (pipes[i].outputMode == OM_RADIO)?"Radio":"Cable",
              clkSrcStr[pipes[i].clkSrc]);
      printDbgBuf(i, bufSize, bufPtr);
    }
  printSAIBufs(bufSize, bufPtr);
}

static void convertMsg(void *data)
{
  int pipeNum = (int)data;
  convert(pipeNum);
}


void aConvInit(void)
{
  bQueueInit(&inQueueUSB1, QUEUE_USB_LIMITS, USBTrimm);
  bQueueInit(&inQueueUSB2, QUEUE_USB_LIMITS, USBTrimm);
  bQueueInit(&inQueueSPI1, QUEUE_SPI1_LIMITS, SPI1Trimm);
  bQueueInit(&inQueueSPI2, QUEUE_SPI2_LIMITS, SPI2Trimm);
  bQueueInit(&inQueueI2S, QUEUE_I2S_LIMITS, I2STrimm);
  bQueueInit(&inQueueTest, QUEUE_TEST_LIMITS, testTrimm);
  bQueueInit(&outQueueSAI1, QUEUE_SAI1_LIMITS, fillUpSAI1);
  bQueueInit(&outQueueSAI2, QUEUE_SAI2_LIMITS, fillUpSAI2);
  pipes[0].outputMode = (cfg.proto & 0x08)?OM_CABLE:OM_RADIO;
  pipes[1].outputMode = (cfg.proto & 0x08)?OM_CABLE:OM_RADIO;
  moduleInit(module);
}



static void __attribute__((optimize(OPTIM))) convert(int pipeNum)
{
  pipe_t *pipe = &pipes[pipeNum];
  if(!pipe->convertFormat || !pipe->convertFormat->convert)
    return;
  int full = 0;
  if (pipe->outBuffer1 == NULL)                 pipe->outBuffer1 = bAlloc(AUDIO_BUFFER_SAMPLES * sizeof(uint64_t));
  if (pipe->outQueue2 != NULL && pipe->outBuffer2 == NULL)
    pipe->outBuffer2 = bAlloc(AUDIO_BUFFER_SAMPLES * sizeof(uint64_t));

  if (pipe->outBuffer1 && (!pipe->outQueue2 || pipe->outBuffer2))
    {
      while (!full)
        {
          if (pipe->inBuffer == NULL)
            pipe->inBuffer =  bDequeue(pipe->inQueue);

          if (pipe->inBuffer != NULL)
            {
              full = pipe->convertFormat->convert(pipeNum, pipe->inBuffer, pipe->outBuffer1, pipe->outBuffer2, pipe->skipRatio);
              if ((pipe->inBuffer)->size == 0)
                {
                  bFree(pipe->inBuffer);
                  pipe->inBuffer = NULL;
                }
            }
          else
            {
              //Buffer not full and input data depleted. Wait for next frame
              return;
            }
        }
      if(pipe->mute == 0 && pipes->unmuteIgnore < xTaskGetTickCount())
        {
          //Buffers are full. Send to transmitters. Reuse if enqueue failed
          if (bEnqueue(pipe->outQueue1, pipe->outBuffer1))
            bFree(pipe->outBuffer1);
          pipe->outBuffer1 = NULL;
          if (pipe->outBuffer2)
            {
              if (bEnqueue(pipe->outQueue2, pipe->outBuffer2))
                bFree(pipe->outBuffer2);
              pipe->outBuffer2 = NULL;
            }
        }
      else
        {
          //Mute. Release buffers without sending to slave
          bFree(pipe->outBuffer1);
          pipe->outBuffer1 = NULL;
          if (pipe->outBuffer2)
            {
              bFree(pipe->outBuffer2);
              pipe->outBuffer2 = NULL;
            }
        }
    }
}


#if defined(CONVERT_DBG)
static uint8_t convertBuf[2][8+8+8+8];
static uint8_t inBufSize[2] = {0,0};
static int setDbgBuf[2] = {1};
static void printDbgBuf(int pipeNum, int *bufSize, char *bufPtr)
{
  if(setDbgBuf[pipeNum] == 1)
    return;
  int i;
  DPRINTF(" Input buffers:  ");
  for(i = 0; i<16; i++)
    {
      if((i%8)<inBufSize[pipeNum])
        {
          DPRINTF("%02x", convertBuf[pipeNum][i]);
        }
      else
        {
          DPRINTF("  ");
        }
      if((i%8) == 7) DPRINTF("     ");
    }
  DPRINTF(CLEAR_LINE"\n Output buffers: ");
  for(i = 16; i<32; i++)
    {
      DPRINTF("%02x", convertBuf[pipeNum][i]);
      if((i%8) == 7) DPRINTF("     ");
    }
  DPRINTF(CLEAR_LINE"\n SAI interfaces: ");
  for(i = 16; i<32; i++)
    {
      DPRINTF("%02x", convertBuf[pipeNum][(i&1)?i&0xfe:i|1]);
      if((i%2) == 1)
        {
          DPRINTF(" ");
        }
      if((i%8) == 7)
        {
          DPRINTF(" ");
        }
    }
  DPRINTF(CLEAR_LINE"\n");
  setDbgBuf[pipeNum] = 1;
}
#define CONVERT_DBGBUF1(arg_inType, arg_outType)                        \
  if(setDbgBuf[pipeNum] == 1){                                                  \
    inBufSize[pipeNum] = sizeof(arg_inType);                            \
    memset(convertBuf[pipeNum], 0, sizeof(convertBuf[pipeNum]));        \
    memcpy(&convertBuf[pipeNum][0],  &inPtr[0].byte[0], sizeof(arg_inType)); \
    memcpy(&convertBuf[pipeNum][16], &outPtr1[0].byte[0], sizeof(arg_outType)); \
    if(outBuffer2)                                                      \
      memcpy(&convertBuf[pipeNum][24], &outPtr1[0].byte[0], sizeof(arg_outType)); \
    setDbgBuf[pipeNum] = 0;                                                     \
  }

#define CONVERT_DBGBUF2(arg_inType, arg_outType)                        \
  if(setDbgBuf[pipeNum] == 1){                                                  \
    inBufSize[pipeNum] = sizeof(arg_inType);                            \
    memset(convertBuf[pipeNum], 0, sizeof(convertBuf[pipeNum]));        \
    memcpy(&convertBuf[pipeNum][0],  &inPtr[0].byte[0], sizeof(arg_inType)); \
    memcpy(&convertBuf[pipeNum][8],  &inPtr[1].byte[0], sizeof(arg_inType)); \
    memcpy(&convertBuf[pipeNum][16], &outPtr1[0].byte[0], sizeof(arg_outType)); \
    memcpy(&convertBuf[pipeNum][24], &outPtr2[0].byte[0], sizeof(arg_outType)); \
    setDbgBuf[pipeNum] = 0;                                                     \
  }
#else
#define CONVERT_DBGBUF1(arg_inType, arg_outType)
#define CONVERT_DBGBUF2(arg_inType, arg_outType)
#endif
#define MIN(a, b) (((a)<(b))?(a):(b))
#define DATA_TYPE(arg_Type, arg_dataSize)                               \
  typedef union                                                         \
  {                                                                     \
    uint8_t byte[arg_dataSize];                                         \
  }__attribute__((packed))arg_Type

#define CONVERT_ROUTINE1(arg_name, arg_inType, arg_outType, arg_actions) \
  static CONV_RET __attribute__((optimize(OPTIM))) arg_name(CONV_PARAMS) \
  {                                                                     \
    arg_inType  *inPtr   = (void*)&inBuffer->data[inBuffer->offset];    \
    arg_outType *outPtr1 = (void*)&outBuffer1->data[outBuffer1->offset]; \
                                                                        \
    int numOut  = MIN((inBuffer->size/skipRatio)/sizeof(arg_inType),    \
                      (outBuffer1->maxSize-outBuffer1->size)/           \
                      sizeof(arg_outType));                             \
    int numIn = skipRatio*numOut;                                       \
    if(!numOut)                                                         \
      {                                                                 \
        inBuffer->size = 0;/*Release the buffer*/                       \
        /*Reuse output buffer*/                                         \
        outBuffer1->size = 0;                                           \
        outBuffer1->offset = 0;                                         \
        return 0;                                                       \
      }                                                                 \
    massert(outBuffer1->size <= outBuffer1->maxSize);                   \
    outBuffer1->size   += numOut*sizeof(arg_outType);                   \
    inBuffer->size     -= numIn*sizeof(arg_inType);                     \
    outBuffer1->offset += numOut*sizeof(arg_outType);                   \
    inBuffer->offset   += numIn*sizeof(arg_inType);                     \
    massert(outBuffer1->size <= outBuffer1->maxSize);                   \
    while(numIn)                                                        \
      {                                                                 \
        numIn-=1*skipRatio;                                             \
        numOut--;                                                       \
        arg_actions;                                                    \
      }                                                                 \
    if(outBuffer2 && outBuffer1->size == outBuffer1->maxSize)           \
      {                                                                 \
        /*Second buffer is a copy*/                                     \
        outBuffer2->size = outBuffer1->size;                            \
        outBuffer2->offset = outBuffer1->offset;                        \
        memcpy(outBuffer2->data, outBuffer1->data, outBuffer2->size);   \
      }                                                                 \
    CONVERT_DBGBUF1(arg_inType, arg_outType);                           \
    return (outBuffer1->size == outBuffer1->maxSize);/*Full or not*/    \
  }

#define CONVERT_ROUTINE2(arg_name, arg_inType, arg_outType, arg_actions) \
  static CONV_RET __attribute__((optimize(OPTIM))) arg_name(CONV_PARAMS) \
  {                                                                     \
    arg_inType  *inPtr   = (void*)&inBuffer->data[inBuffer->offset];    \
    arg_outType *outPtr1 = (void*)&outBuffer1->data[outBuffer1->offset]; \
    arg_outType *outPtr2 = (void*)&outBuffer2->data[outBuffer2->offset]; \
                                                                        \
    int numOut = MIN((inBuffer->size/(2*skipRatio))/sizeof(arg_inType), \
                     (outBuffer1->maxSize-outBuffer1->size)/            \
                     sizeof(arg_outType));                              \
    int numIn = skipRatio*2*numOut;                                     \
    if(!numOut)                                                         \
      {                                                                 \
        inBuffer->size = 0;/*Release the buffer*/                       \
        /*Reuse output buffers*/                                        \
        outBuffer1->size = 0;                                           \
        outBuffer1->offset = 0;                                         \
        outBuffer2->size = 0;                                           \
        outBuffer2->offset = 0;                                         \
        return 0;                                                       \
      }                                                                 \
    outBuffer1->size   +=   numOut*sizeof(arg_outType);                 \
    outBuffer2->size   +=   numOut*sizeof(arg_outType);                 \
    inBuffer->size     -=   numIn*sizeof(arg_inType);                   \
    outBuffer1->offset +=   numOut*sizeof(arg_outType);                 \
    outBuffer2->offset +=   numOut*sizeof(arg_outType);                 \
    inBuffer->offset   +=   numIn*sizeof(arg_inType);                   \
    massert(numIn);                                                     \
    massert(outBuffer1->size <= outBuffer1->maxSize);                   \
    while(numIn)                                                        \
      {                                                                 \
        numIn-=2*skipRatio;                                             \
        numOut-=1;                                                      \
        arg_actions;                                                    \
      }                                                                 \
    CONVERT_DBGBUF2(arg_inType, arg_outType);                           \
    return (outBuffer1->size == outBuffer1->maxSize);/*Full or not*/    \
  }

//32 bit
DATA_TYPE(t_16L16P, 4);

//48 bit
DATA_TYPE(t_24L24P, 6);

//64 bit
DATA_TYPE(t_16L16P32N,  8);
DATA_TYPE(t_24L24P16N,  8);
DATA_TYPE(t_16L16P8L8P16N, 8);
DATA_TYPE(t_24L8N24P8N, 8);
DATA_TYPE(t_16L16N16P16N, 8);
DATA_TYPE(t_16L16L32N,    8);
DATA_TYPE(t_24L24L16N,    8);
DATA_TYPE(t_16L8L8N16P8P8N, 8);
DATA_TYPE(t_16L16L8L8L16N, 8);

#define SET_SLOT(arg_sai, arg_slot, arg0, arg1)\
  outPtr##arg_sai[numOut].byte[2*(arg_slot) + 0] = inPtr[numIn].byte[arg1]; \
  outPtr##arg_sai[numOut].byte[2*(arg_slot) + 1] = inPtr[numIn].byte[arg0]

CONVERT_ROUTINE1(conv_ns_16L16P_to_16L16P32N, t_16L16P, t_16L16P32N,
{
  //10 32
  SET_SLOT(1, 0, 1, 0);
  SET_SLOT(1, 1, 3, 2);
})

CONVERT_ROUTINE1(conv_ns_16L16N16P16N_to_16L16P32N, t_16L16N16P16N, t_16L16P32N,
{
  //10 54
  SET_SLOT(1, 0, 1, 0);
  SET_SLOT(1, 1, 5, 4);
})

CONVERT_ROUTINE2(conv_ds_16L16P_to_16L16L32N_16P16P32N, t_16L16P, t_16L16L32N,
{
  //10 32 | 54 76
  SET_SLOT(1, 0, 1, 0);
  SET_SLOT(1, 1, 5, 4);
  SET_SLOT(2, 0, 3, 2);
  SET_SLOT(2, 1, 7, 6);
})
CONVERT_ROUTINE2(conv_ds_16L16N16P16N_to_16L16L32N_16P16P32N, t_16L16N16P16N, t_16L16L32N,
{
  //10 54 | 98 DC
  SET_SLOT(1, 0, 1, 0);
  SET_SLOT(1, 1, 9, 8);
  SET_SLOT(2, 0, 5, 4);
  SET_SLOT(2, 1, 13, 12);
})

//Tryb multislave radiowy 24 bit
CONVERT_ROUTINE1(conv_ns_24L24P_to_16L16P32N, t_24L24P, t_16L16P32N,
{
  //210 543
  SET_SLOT(1, 0, 2, 1);
  SET_SLOT(1, 1, 5, 4);
})
CONVERT_ROUTINE1(conv_ns_24L8N24P8N_to_16L16P32N, t_24L8N24P8N, t_16L16P32N,
{
  //321 765
  SET_SLOT(1, 0, 3, 2);
  SET_SLOT(1, 1, 7, 6);
})
CONVERT_ROUTINE2(conv_ds_24L24P_to_16L16L32N_16P16P32N, t_24L24P, t_16L16L32N,
{
  //210 543
  SET_SLOT(1, 0, 2, 1);
  SET_SLOT(2, 0, 5, 4);
})
CONVERT_ROUTINE2(conv_ds_24L8N24P8N_to_16L16L32N_16P16P32N, t_24L8N24P8N, t_16L16L32N,
{
  //321 765
  SET_SLOT(1, 0, 3, 2);
  SET_SLOT(2, 0, 7, 6);
})
CONVERT_ROUTINE1(conv_ns_16L8L8N16P8P8N_to_16L16P32N, t_16L8L8N16P8P8N, t_16L16P32N,
{
  //103 547
  SET_SLOT(1, 0, 1, 0);
  SET_SLOT(1, 1, 5, 4);
})
CONVERT_ROUTINE2(conv_ds_16L8L8N16P8P8N_to_16L16L32N_16P16P32N, t_16L8L8N16P8P8N, t_16L16L32N,
{
  //103 547
  SET_SLOT(1, 0, 1, 0);
  SET_SLOT(2, 0, 5, 4);
})

//Tryb kablowy lub FullHD 24bit
CONVERT_ROUTINE1(conv_ns_24L24P_to_24L24P16N, t_24L24P, t_24L24P16N,
{
  //210 543
  SET_SLOT(1, 0, 2, 1);
  SET_SLOT(1, 1, 0, 5);
  SET_SLOT(1, 2, 4, 3);
})
CONVERT_ROUTINE1(conv_ns_24L24P_to_16L16P8L8P16N_16L16P8L8P16N, t_24L24P, t_16L16P8L8P16N,
{
  //210 543
  SET_SLOT(1, 0, 2, 1);
  SET_SLOT(1, 1, 5, 4);
  SET_SLOT(1, 2, 0, 3);
})
CONVERT_ROUTINE1(conv_ns_24L8N24P8N_to_24L24P16N, t_24L8N24P8N, t_24L24P16N,
{
  //321 765
  SET_SLOT(1, 0, 3, 2);
  SET_SLOT(1, 1, 1, 7);
  SET_SLOT(1, 2, 6, 5);
})
CONVERT_ROUTINE1(conv_ns_16L8L8N16P8P8N_to_24L24P16N, t_16L8L8N16P8P8N, t_24L24P16N,
{
  //103 547
  SET_SLOT(1, 0, 1, 0);
  SET_SLOT(1, 1, 3, 5);
  SET_SLOT(1, 2, 4, 7);
})
CONVERT_ROUTINE1(conv_ns_16L8L8N16P8P8N_to_16L16P8L8P16N_16L16P8L8P16N, t_16L8L8N16P8P8N, t_16L16P8L8P16N,
{
  //103 547
  SET_SLOT(1, 0, 1, 0);
  SET_SLOT(1, 1, 5, 4);
  SET_SLOT(1, 2, 3, 7);
})
CONVERT_ROUTINE2(conv_ds_24L24P_to_24L24L16N_24P24P16N, t_24L24P, t_24L24L16N,
{
  //210 543 | 876 BA9
  SET_SLOT(1, 0, 2, 1);
  SET_SLOT(1, 1, 0, 8);
  SET_SLOT(1, 2, 7, 6);

  SET_SLOT(2, 0, 5, 4);
  SET_SLOT(2, 1, 3, 11);
  SET_SLOT(2, 2, 10, 9);
})
CONVERT_ROUTINE2(conv_ds_24L24P_to_16L16L8L8L16N_16P16P8P8P16N, t_24L24P, t_16L16L8L8L16N,
{
  //210 543 | 876 BA9
  SET_SLOT(1, 0, 2, 1);
  SET_SLOT(1, 1, 8, 7);
  SET_SLOT(1, 2, 0, 6);

  SET_SLOT(2, 0, 5, 4);
  SET_SLOT(2, 1, 11, 10);
  SET_SLOT(2, 2, 3, 9);
})
CONVERT_ROUTINE2(conv_ds_24L8N24P8N_to_24L24L16N_24P24P16N, t_24L8N24P8N, t_24L24L16N,
{
  //321 765 | BA9 FED
  SET_SLOT(1, 0, 3, 2);
  SET_SLOT(1, 1, 1, 11);
  SET_SLOT(1, 2, 10, 9);

  SET_SLOT(2, 0, 7, 6);
  SET_SLOT(2, 1, 5, 15);
  SET_SLOT(2, 2, 14, 13);
})
CONVERT_ROUTINE2(conv_ds_24L8N24P8N_to_16L16L8L8L16N_16P16P8P8P16N, t_24L8N24P8N, t_16L16L8L8L16N,
{
  //321 765 | BA9 FED
  SET_SLOT(1, 0, 3, 2);
  SET_SLOT(1, 1, 11, 10);
  SET_SLOT(1, 2, 1, 9);

  SET_SLOT(2, 0, 7, 6);
  SET_SLOT(2, 1, 15, 14);
  SET_SLOT(2, 2, 5, 13);
})
CONVERT_ROUTINE2(conv_ds_16L8L8N16P8P8N_to_24L24L16N_24P24P16N, t_16L8L8N16P8P8N, t_24L24L16N,
{
  //103 547  | 98B DCF
  SET_SLOT(1, 0, 1, 0);
  SET_SLOT(1, 1, 3, 9);
  SET_SLOT(1, 2, 8, 11);

  SET_SLOT(2, 0, 5, 4);
  SET_SLOT(2, 1, 7, 13);
  SET_SLOT(2, 2, 12, 15);
})
CONVERT_ROUTINE2(conv_ds_16L8L8N16P8P8N_to_16L16L8L8L16N_16P16P8P8P16N, t_16L8L8N16P8P8N, t_16L16L8L8L16N,
{
  //103 547  | 98B DCF
  SET_SLOT(1, 0, 1, 0);
  SET_SLOT(1, 1, 9, 8);
  SET_SLOT(1, 2, 3, 11);

  SET_SLOT(2, 0, 5, 4);
  SET_SLOT(2, 1, 13, 12);
  SET_SLOT(2, 2, 7, 15);
})

static CONV_RET __attribute__((optimize(OPTIM))) conv_NULL(CONV_PARAMS)
{
  if(outBuffer1) outBuffer1->size = outBuffer1->maxSize;
  if(outBuffer2) outBuffer2->size = outBuffer2->maxSize;
  inBuffer->size                  = 0;
  return 1;
}
