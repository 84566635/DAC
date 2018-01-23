#include <project.h>
#include <common.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "math.h"
#include STMINCP()
#include STMINCS(_usart)
#include "startup.h"
#ifdef __GNUC__

void xPortSysTickHandler(void);
static uint64_t uptime = 0;
static uint64_t preciseUptime = 0;

void sysTickHandler(void)
{
  uptime += 1000000ULL;
  preciseUptime = uptime;
  xPortSysTickHandler();
}
static inline uint64_t sysTickOffset(void)
{
  volatile uint32_t tick = (uint32_t)SysTick->VAL;
  return (uint64_t)((((SystemCoreClock/1000UL) - tick)*1000UL)/(SystemCoreClock/1000000UL));
}

uint64_t getUptimePrecise(void)
{
  uint64_t prevUptime = 0;
  int mask =  portSET_INTERRUPT_MASK_FROM_ISR();
  prevUptime = preciseUptime;
  preciseUptime = uptime + sysTickOffset();

  if(prevUptime > preciseUptime)
    preciseUptime += 1000000ULL;

  portCLEAR_INTERRUPT_MASK_FROM_ISR(mask);

  return preciseUptime;
}

uint64_t getUptime(void)
{
  return uptime;
}

int disableIrq(void)
{
  int primask = 0;
  asm volatile ("MRS %0, primask \r\n" "cpsid i \r\n":"=r" (primask));

  return primask & 1;
}
int getIrqNum(void)
{
  uint32_t irqNum;
  asm volatile ("mrs %0, ipsr":"=r" (irqNum));
  return irqNum;
}
#else
__asm int getIrqNum( void )
{
  PRESERVE8

  mrs r0, ipsr
  bx r14
}

__asm int disableIrq(void)
{
  PRESERVE8
  mrs r0, primask
  cpsid i
  bx r14
}
#endif
void enableIrq(int primask)
{
  if (!primask)
    __asm volatile ("cpsie i");
}
/*-----------------------------------------------------------*/
void vApplicationStackOverflowHook(xTaskHandle pxTask, signed char *pcTaskName);
void vApplicationStackOverflowHook(xTaskHandle pxTask, signed char *pcTaskName)
{

  /* This function will get called if a task overflows its stack.   If the
     parameters are corrupt then inspect pxCurrentTCB to find which was the
     offending task. */
  (void)pxTask;
  (void)pcTaskName;
  {
    volatile int s = 1;
    while (s);
  }
}

void vApplicationMallocFailedHook(void);
void vApplicationMallocFailedHook(void)
{
  dprintf(LL_ERROR, "Heap depleted\n");
  {
    volatile int s = 1;
    while (s);
  }
}

void HardFault_Handler(void);
#if 1
void  __attribute__ ((optimize("-O0"))) getRegistersFromStack( uint32_t *pulFaultStackAddress );
void  __attribute__ ((optimize("-O0"))) getRegistersFromStack( uint32_t *pulFaultStackAddress )
{
  uint32_t sptr;
  asm("mov %0, sp": "=r" (sptr));
  dprintf(LL_ERROR, "HARDFAULT!!\n");
  dprintf(LL_ERROR, "r0   %08x r1  %08x r2  %08x r3  %08x \n", pulFaultStackAddress[0], pulFaultStackAddress[1], pulFaultStackAddress[2], pulFaultStackAddress[3]);
  dprintf(LL_ERROR, "r12  %08x lr  %08x pc  %08x psr %08x \n", pulFaultStackAddress[4], pulFaultStackAddress[5], pulFaultStackAddress[6], pulFaultStackAddress[7]);
  dprintf(LL_ERROR, "sp   %08x\n", sptr);
  {
    volatile int s = 1;
    while (s);
  }
}
void __attribute__ ((optimize("-O0"),no_instrument_function,naked)) HardFault_Handler(void)
{
  __asm volatile
  (
    " tst lr, #4\n"
    " ite eq\n"
    " mrseq r0, msp\n"
    " mrsne r0, psp\n"
    " ldr r1, [r0, #24]\n"
    " ldr r2, handler2_address_const\n"
    " bx r2\n"
    " handler2_address_const: .word getRegistersFromStack \n"
  );
}


#else
void HardFault_Handler(void)
{
  dprintf(LL_ERROR, "HARDFAULT!!\n");
  {
    volatile int s = 1;
    while (s);
  }
}
#endif

void *intSafeMalloc(int size)
{
  int irq = amIInIRQ();
  if(!irq) taskENTER_CRITICAL();
  void *buf =  pvPortMalloc(size);
  if(!irq) taskEXIT_CRITICAL();
  return buf;
}
void intSafeFree(void *buf)
{
  int irq = amIInIRQ();
  if(!irq) taskENTER_CRITICAL();
  vPortFree(buf);
  if(!irq) taskEXIT_CRITICAL();
}

uint8_t refresh = 0;
#ifdef DEBUG_UART
static int consoleLocked = 0;
void consoleLock(void)
{
  taskENTER_CRITICAL();
  while(consoleLocked)
    {
      taskEXIT_CRITICAL();
      mdelay(1);
      taskENTER_CRITICAL();
    }
  consoleLocked = 1;
  taskEXIT_CRITICAL();
}

void consoleUnlock(void)
{
  taskENTER_CRITICAL();
  consoleLocked = 0;
  taskEXIT_CRITICAL();
}

void putChar(unsigned char ch)
{
  while(!USART_GetFlagStatus(DEBUG_UART, USART_FLAG_TXE));
  USART_SendData(DEBUG_UART, ch);
}

int getChar(void)
{
  if(USART_GetFlagStatus(DEBUG_UART, USART_FLAG_RXNE))
    return USART_ReceiveData(DEBUG_UART);
  return -1;
}
void sendChar(unsigned char ch)
{
  consoleLock();
  putChar(ch);
  if(ch == '\n')refresh = 1;
  consoleUnlock();

}
#endif
void hexDump(void* ptr, int size, int rowSize)
{
  int cnt = 0;
  int row = 0;
  for(cnt=0; cnt<size; cnt++)
    {
      if((cnt%rowSize) == 0)
        xprintf("\n%10p (%02X)[%02X]: ",ptr, cnt, row++);
      xprintf("%02X ",((uint8_t*)ptr)[cnt]);
    }
}

static int taskInitWait = 1;
void taskWaitInit(void)
{
  while(taskInitWait)
    mdelay(1);
}

void tasksInitEnd(void)
{
  taskInitWait = 0;
}

