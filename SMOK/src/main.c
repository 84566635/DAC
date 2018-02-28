#include <project.h>
#include <common.h>
#include <string.h>

#include STMINCP()
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "misc.h"
#include <hwCommunicator.h>
#include <hwCommunicatorUARThelper.h>
#include <hwCommunicatorSPIhelper.h>
#include <communicator.h>
#include <led.h>
#include <adc.h>
#include <startup.h>
#include <bqueue.h>
#include <debug.h>
#include <watchdog.h>
#include "dacVolume.h"


///////////////////////////////////////////////////
/*-----------------------------------------------------------*/
void systemInit(void)
{
  hwBoardInit();

  void messageProcessorInit(void);
  messageProcessorInit();

  /* Initialize tasks */
  LEDInit();
  ADCInit();
  spiHelperInit();
  CommunicatorInit();
  HWCommInit();
  debugInit();
  DACVOLUME_Init();
  tasksInitEnd();
}
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
