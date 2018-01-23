#include <project.h>
#include <common.h>
#include <string.h>
#include <math.h>
#include <misc.h>

#include STMINCP()
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>

#include <hwCommunicator.h>
#include <hwCommunicatorUARThelper.h>
#include <hwCommunicatorSPIhelper.h>
#include <communicator.h>
#include <led.h>
#include <adc.h>
#include <startup.h>


#include <bqueue.h>
#include <convert.h>
#include <audioCore.h>
#include <saihwInit.h>
#include <debug.h>
#include <messageProcessorMADO.h>
#include <watchdog.h>


#define TEST_RATES 6
static const int testRate[TEST_RATES] =
{
  [0] =  44100,
  [1] =  48000,
  [2] =  88200,
  [3] =  96000,
  [4] =  176400,
  [5]  = 192000,
} ;

#define LEN 768

#define SET4(arg1, arg2, arg3, arg4)            \
  for (i = 0; i < LEN; i++)                     \
    switch(i%4)                                 \
      {                                         \
      case 0:buf->data[i] = arg1;break;         \
      case 1:buf->data[i] = arg2;break;         \
      case 2:buf->data[i] = arg3;break;         \
      case 3:buf->data[i] = arg4;break;         \
      }

#define SET6(arg1, arg2, arg3, arg4, arg5, arg6)        \
  for (i = 0; i < LEN; i++)                             \
    switch(i%6)                                         \
      {                                                 \
      case 0:buf->data[i] = arg1;break;                 \
      case 1:buf->data[i] = arg2;break;                 \
      case 2:buf->data[i] = arg3;break;                 \
      case 3:buf->data[i] = arg4;break;                 \
      case 4:buf->data[i] = arg5;break;                 \
      case 5:buf->data[i] = arg6;break;                 \
      }

#define SET8(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)            \
  for (i = 0; i < LEN; i++)                                             \
    switch(i%8)                                                         \
      {                                                                 \
      case 0:buf->data[i] = arg1;break;                                 \
      case 1:buf->data[i] = arg2;break;                                 \
      case 2:buf->data[i] = arg3;break;                                 \
      case 3:buf->data[i] = arg4;break;                                 \
      case 4:buf->data[i] = arg5;break;                                 \
      case 5:buf->data[i] = arg6;break;                                 \
      case 6:buf->data[i] = arg7;break;                                 \
      case 7:buf->data[i] = arg8;break;                                 \
      }
#define SET12(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12) \
  for (i = 0; i < LEN; i++)                                             \
    switch(i%12)                                                         \
      {                                                                 \
      case 0:buf->data[i] = arg1;break;                                 \
      case 1:buf->data[i] = arg2;break;                                 \
      case 2:buf->data[i] = arg3;break;                                 \
      case 3:buf->data[i] = arg4;break;                                 \
      case 4:buf->data[i] = arg5;break;                                 \
      case 5:buf->data[i] = arg6;break;                                 \
      case 6:buf->data[i] = arg7;break;                                 \
      case 7:buf->data[i] = arg8;break;                                 \
      case 8:buf->data[i] = arg9;break;                                 \
      case 9:buf->data[i] = arg10;break;				\
      case 10:buf->data[i] = arg11;break;				\
      case 11:buf->data[i] = arg12;break;				\
      }
#define SET16(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15, arg16) \
  for (i = 0; i < LEN; i++)                                             \
    switch(i%16)							\
      {                                                                 \
      case 0:buf->data[i] = arg1;break;                                 \
      case 1:buf->data[i] = arg2;break;                                 \
      case 2:buf->data[i] = arg3;break;                                 \
      case 3:buf->data[i] = arg4;break;                                 \
      case 4:buf->data[i] = arg5;break;                                 \
      case 5:buf->data[i] = arg6;break;                                 \
      case 6:buf->data[i] = arg7;break;                                 \
      case 7:buf->data[i] = arg8;break;                                 \
      case 8:buf->data[i] = arg9;break;                                 \
      case 9:buf->data[i] = arg10;break;				\
      case 10:buf->data[i] = arg11;break;				\
      case 11:buf->data[i] = arg12;break;				\
      case 12:buf->data[i] = arg13;break;				\
      case 13:buf->data[i] = arg14;break;				\
      case 14:buf->data[i] = arg15;break;				\
      case 15:buf->data[i] = arg16;break;				\
      }

static int testInputPatternNum = 0;
void testTrimm(unsigned int count)
{

  bBuffer_t *buf = bAlloc(LEN);
  int i;
  buf->size = LEN;
  switch (testInputPatternNum)
    {
    case 0 ... 1:
      // IF_16L16P normal sampling
      SET4(0x01, 0x02, 0x11, 0x12)
      break;
    case 2 ... 5:
      // IF_16L16P half sampling
      SET8(0x01, 0x02, 0x11, 0x12, 0x81, 0x82, 0x91, 0x92)
      break;
    case 6 ... 7:
      //IF_24L24P normal sampling
      SET6(0x01, 0x02, 0x03, 0x11, 0x12, 0x13)
      break;
    case 8 ... 11:
      //IF_24L24P half sampling
      SET12(0x01, 0x02, 0x03, 0x11, 0x12, 0x13, 0x81, 0x82, 0x83, 0x91, 0x92, 0x93)
      break;
    case 12 ... 13:
      // IF_16L16N16P16N normal sampling
      SET8(0x01, 0x02, 0x00, 0x00, 0x11, 0x12, 0x00, 0x00)
      break;
    case 14 ... 17:
      // IF_16L16N16P16N half sampling
      SET16(0x01, 0x02, 0x00, 0x00, 0x11, 0x12, 0x00, 0x00, 0x81, 0x82, 0x00, 0x00, 0x91, 0x92, 0x00, 0x00)
      break;
    case 18 ... 19:
      // IF_16L8L8N16P8P8N normal sampling
      SET8(0x02, 0x03, 0x00, 0x01, 0x82, 0x83, 0x00, 0x81)
      break;
    case 20 ... 23:
      // IF_16L8L8N16P8P8N half sampling
      SET16(0x02, 0x03, 0x00, 0x01, 0x12, 0x13, 0x00, 0x11, 0x82, 0x83, 0x00, 0x81, 0x92, 0x93, 0x00, 0x91)
      break;
    }


  if (bEnqueue(&inQueueTest, buf) < 0)
    bFree(buf);
}

int testInputPatternChange(int pattern)
{
  testInputPatternNum = pattern;
  switch (testInputPatternNum)
    {
    case 0 ... 5:
      formatChange(INPUT_TEST, IF_16L16P, testRate[testInputPatternNum%TEST_RATES]);
      break;
    case 6 ... 11:
      formatChange(INPUT_TEST, IF_24L24P, testRate[testInputPatternNum%TEST_RATES]);
      break;
    case 12 ... 17:
      formatChange(INPUT_TEST, IF_16L16N16P16N, testRate[testInputPatternNum%TEST_RATES]);
      break;
    case 18 ... 23:
      formatChange(INPUT_TEST, IF_16L8L8N16P8P8N, testRate[testInputPatternNum%TEST_RATES]);
      break;
    }
  return (pattern+1)%TEST_PATTERNS_NUM;
}


void systemInit(void)
{
  hwBoardInit();
  LEDInit();

  void messageProcessorInit(void);
  messageProcessorInit();

  /* Initialize tasks */

  spiHelperInit();
  CommunicatorInit();
  HWCommInit();
  saiInit();
  aConvInit();

  debugInit();
  tasksInitEnd();

}
/*-----------------------------------------------------------*/
int main(void)
{

  /* Set up the clocks and memory interface. */
  /* Initialize HW ports, ADCs. LEDs */
  NVIC_PriorityGroupConfig( NVIC_PriorityGroup_4 );
  SystemInit();

  watchdogInit();


  vTaskStartScheduler();
  for (;;);
}
