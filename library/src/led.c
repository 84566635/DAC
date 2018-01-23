#include <project.h>
#include <common.h>
#include STMINCP()
#include STMINCS(_gpio)
#include <bqueue.h>
#include <moduleCommon.h>
#include <led.h>
#include "FreeRTOS.h"
#include <task.h>
#include <watchdog.h>

enum
{
  CMD_LED_BLINK,
  CMD_LED_SET,
};

static void ledBlink(void *data);
static void ledSet(void *data);
static void wdog(void *data);

static const action_t commands[] =
{
  [CMD_LED_BLINK] = ledBlink,
  [CMD_LED_SET]   = ledSet,
};
static moduleModule_t module[LED_BLINKER_NUM] =
{
  [0 ... LED_BLINKER_NUM - 1] = {
    .queueLength = 16,
    .timeout = 100,               //[ms]
    .commandsNum = sizeof(commands) / sizeof(commands[0]),
    .initCallback = NULL,
    .commandCallback = commands,
    .timeoutCallback = NULL,
    .alwaysCallback = wdog,
    .priority = 4,
    .stackSize = 256,
    .name = "LED blinker",
    .privateData = (void*)1,
  }
};

static void wdog(void *data)
{
#ifdef WATCHDOG_LED
  int i;
  for(i=0; i < LED_BLINKER_NUM; i++)
    if(&module[i] == data)
      wdogFeed(WATCHDOG_LED(0));
#endif
}

static uint32_t ledsState = 0;
void LEDInit(void)
{
  //Initialize LED blinker
  LEDSset(0, ~0);
  ledsState = 0;
  int blinker;
  for (blinker = 0; blinker < LED_BLINKER_NUM; blinker++)
    {
      moduleInit(&(module[blinker]));
    }
}


#define LOOP_COUNT(arg_blinker) ((uint32_t)module[arg_blinker].privateData)
void LEDBreakLoop(int LEDblinker)
{
  if (LEDblinker >= LED_BLINKER_NUM)
    return;
  module[LEDblinker].privateData = (void*)(LOOP_COUNT(LEDblinker) + 1);
}

/////////////////////////////////////////////////////////////////////
typedef struct
{
  int LEDblinker;
  uint32_t ledsMask;
  int blinkNum;
  int time_on;
  int time_break;
  int time_pre;
  int time_post;
  uint32_t loop;
} messageLEDData_t;
static int Bnum[LED_BLINKER_NUM];
int LEDblink(int LEDblinker, uint32_t ledsMask, int blinkNum, int time_on, int time_break, int time_pre, int time_post, uint8_t loop)
{
  if (LEDblinker >= LED_BLINKER_NUM)
    return -1;
  if (blinkNum == LED_BLINK_STOP)
    {
      Bnum[LEDblinker] = LED_BLINK_STOP;
    }
  else
    {
      //Allocate resources for data transfer
      messageLEDData_t *message = dAlloc(sizeof(messageLEDData_t));

      if (message == NULL)
        return -1;

      //Fill message
      message->LEDblinker = LEDblinker;
      message->ledsMask = ledsMask;
      message->blinkNum = blinkNum;
      message->time_on = time_on;
      message->time_break = time_break;
      message->time_pre = time_pre;
      message->time_post = time_post;
      message->loop = loop?LOOP_COUNT(LEDblinker):LOOP_COUNT(LEDblinker)-1;

      //Send the message to communicator
      if (moduleSendCommand(&module[LEDblinker], CMD_LED_BLINK, message) < 0)
        {
          //Sending failed
          //Free allocated resources
          dFree(message);
          return -1;
        }
    }
  return 0;
}
typedef struct
{
  int LEDblinker;
  uint32_t ledsMaskOn;
  uint32_t ledsMaskOff;
} messageLEDSetData_t;
static int Bnum[LED_BLINKER_NUM];
int LEDset(int LEDblinker, uint32_t ledsMaskOn, uint32_t ledsMaskOff)
{
  if (LEDblinker >= LED_BLINKER_NUM)
    return -1;

  //Nothing to set
  if(ledsMaskOn == 0 && ledsMaskOff == 0)
    return 0;

  //Allocate resources for data transfer
  messageLEDSetData_t *message = dAlloc(sizeof(messageLEDSetData_t));

  if (message == NULL)
    return -1;

  //Fill message
  message->LEDblinker = LEDblinker;
  message->ledsMaskOn = ledsMaskOn;
  message->ledsMaskOff = ledsMaskOff;

  //Send the message to communicator
  if (moduleSendCommand(&module[LEDblinker], CMD_LED_SET, message) < 0)
    {
      //Sending failed
      //Free allocated resources
      dFree(message);
      return -1;
    }
  return 0;
}

void LEDSset(uint32_t ledsMaskOn, uint32_t ledsMaskOff)
{
  int LEDNum = 0;
  while ((ledsMaskOn || ledsMaskOff) && LEDNum < LEDS_NUM)
    {
      int value  = -1;
      if(ledsMaskOff & 1)value = 0;
      else if(ledsMaskOn & 1)value = 1;
      if (value >= 0)
        {
          if(leds[LEDNum].pin)
            GPIO_WriteBit(leds[LEDNum].gpio, leds[LEDNum].pin, leds[LEDNum].invertPolarity ? !value : value);
          if(LED_print)
            LED_print(LEDNum, value);
        }
      LEDNum++;
      ledsMaskOn >>= 1;
      ledsMaskOff >>= 1;
    }
}

static void ledBlink(void *data)
{
  messageLEDData_t *message = data;

  //Pre blink
  LEDSset(0, message->ledsMask);
  vTaskDelay(message->time_pre);

  //Blink
  int *num = &Bnum[message->LEDblinker];
  *num = message->blinkNum;

  while (*num)
    {
      if (*num != LED_BLINK_INFINITY)
        {
          if (*num != LED_BLINK_STOP)
            (*num)--;
          else
            *num = 0;
        }

      LEDSset(message->ledsMask, 0);
      vTaskDelay(message->time_on);
      LEDSset(0, message->ledsMask);
      //For last blink do not wait for time_break
      if (*num != 0)
        vTaskDelay(message->time_break);
    }
  vTaskDelay(message->time_post);
  LEDSset(message->ledsMask & ledsState, 0);

  if(message->loop == LOOP_COUNT(message->LEDblinker))
    if (moduleSendCommand(&module[message->LEDblinker], CMD_LED_BLINK, message) >= 0)
      return;//Sending ok
  dFree(message);
}

static void ledSet(void *data)
{
  messageLEDSetData_t *message = data;

  LEDSset(message->ledsMaskOn, message->ledsMaskOff);
  ledsState &= ~message->ledsMaskOff;
  ledsState |= message->ledsMaskOn;
  dFree(message);
}
