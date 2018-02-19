#include <project.h>
#include <common.h>
#include <string.h>
#include <math.h>
#include <misc.h>

#include STMINCP()
#include STMINCS(_iwdg)
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>
#include <watchdog.h>
#include <cfg.h>

static int fedMask = 0;
void wdogFeed(int mask)
{
  taskENTER_CRITICAL();
  fedMask |= mask;
  taskEXIT_CRITICAL();
}
void __attribute__((weak))wdogAssert(int value)
{
}
static int resetActive = 0;
void watchdogReset(void)
{
  IWDG_ReloadCounter();
  resetActive = 1;
}

static void watchdogTask(void *pvParameters)
{

  IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
  IWDG_SetPrescaler(IWDG_Prescaler_256);
  IWDG_SetReload(2*256);
  IWDG_Enable();
  IWDG_ReloadCounter();

  systemInit();

  int checkTime = WATCHDOG_TIMEOUT*40;
  while (1)
    {
      wdogAssert(0);
      mdelay(12);
      wdogAssert(1);
      mdelay(13);
      if(!resetActive)
        IWDG_ReloadCounter();
      if(checkTime == 0)
        {
          if(fedMask != WATCHDOG_EXPECTED)
            {
              dprintf(LL_ERROR, "\n!!!!!!Some tasks did not respond (fed mask %x). Turn off the power!!!!!\n", fedMask);
              wdogFail();
            }
          taskENTER_CRITICAL();
          fedMask = 0;
          taskEXIT_CRITICAL();
          checkTime = WATCHDOG_TIMEOUT*40;
        }
      else
        checkTime--;
    }
}
void __attribute__((weak))wdogFail(void)
{
  while(1);
}

void watchdogInit(void)
{
  massert(xTaskCreate(watchdogTask, (signed char *)"Watchdog", 2048 / 4, NULL, 1, NULL) == pdPASS);
}
