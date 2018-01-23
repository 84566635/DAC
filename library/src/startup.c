#include <project.h>
#include <stdint.h>
#include <string.h>
#include "FreeRTOS.h"
#include <task.h>
#define DEFAULT_HANDLER(arg_name) void __attribute__ ((no_instrument_function, weak, alias ("Unused_Handler"), interrupt)) arg_name(void)
#include <startup.h>
#include <common.h>
#include <misc.h>
#include <cfg.h>

struct irqHandler_s;
typedef struct irqHandler_s irqHandler_t;
struct irqHandler_s
{
  irqHandler_t *next;
  void (*handler) (void);
};
static irqHandler_t *handlers[ISR_SIZE]__attribute__((section(".noload")));

#ifndef IRQ_HANDLER_POOL
# define IRQ_HANDLER_POOL ISR_SIZE
#endif

//Stack must be 8 byte aligned
uint64_t common_stack[STACK_SIZE / 8] __attribute__ ((section(".stack"), aligned(8), used));
#define NVICE(arg_name) (uint32_t)(arg_name)

//////////////////////////////////////////////// BOOTLOADER ////////////////////////////////////////////////
static void __attribute__ ((no_instrument_function, interrupt, optimize("-O3"), section(".bootloader"))) resetDefaultIRQHandler(void)
{
  int s=1;
  while(s);
}

void __attribute__ ((no_instrument_function,naked)) Reset_Handler(void);
static void __attribute__ ((no_instrument_function, section(".bootloader"), optimize("-O0"), naked)) Boot_Reset_Handler(void);

extern uint32_t _new_image[2];
static void Boot_Reset_Handler(void)
{
  //Update FW if new one available
  updateFW();

  //Set new interrupt table
  SCB->VTOR = ((uint32_t)_new_image);
  //Set new stack
  __set_MSP(_new_image[0]);

  //Proceed to main image by calling it's reset
  void (*mainImageReset)(void) = (void*)_new_image[1];
  mainImageReset();
  while(1);
}


static const uint32_t resetisr[ISR_SIZE] __attribute__ ((section(".resetisr"), used)) =
{
  /* Unused vector is the default one */
  [0 ... ISR_SIZE - 1] = NVICE(resetDefaultIRQHandler),
  /* ARM Cortex common handlers */
  [0] = NVICE(&common_stack[STACK_SIZE / 8]),
  [1] = NVICE(Boot_Reset_Handler),
};




//////////////////////////////////////////////// NORMAL IMAGE //////////////////////////////////////////////
static void __attribute__ ((no_instrument_function, interrupt, optimize("-O3"))) IRQHandler(void);
static irqHandler_t pool[IRQ_HANDLER_POOL]__attribute__((section(".noload")));
static irqHandler_t *poolFree = pool;

extern uint32_t __etext;
extern uint32_t __sdata;
extern uint32_t __edata;

static const uint32_t isr[ISR_SIZE] __attribute__ ((section(".isr"), used)) =
{
  /* Unused vector is the default one */
  [0 ... ISR_SIZE - 1] = NVICE(IRQHandler),
  /* ARM Cortex common handlers */
  [0] = NVICE(&common_stack[STACK_SIZE / 8]),
  [1] = NVICE(Reset_Handler),
  [2] = NVICE(NMI_Handler),
  [3] = NVICE(HardFault_Handler),
  [4] = NVICE(MemManage_Handler),
  [5] = NVICE(BusFault_Handler),
  [6] = NVICE(UsageFault_Handler),
  [11] = NVICE(vPortSVCHandler),
  [12] = NVICE(DebugMon_Handler),
  [14] = NVICE(xPortPendSVHandler),
  [15] = NVICE(sysTickHandler),
};


void Reset_Handler(void)
{
#ifdef DEBUG
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
#endif

  //Enable FPU
#if (__FPU_USED == 1)
  SCB->CPACR |= ((3UL << 10*2) |                 /* set CP10 Full Access */
                 (3UL << 11*2)  );               /* set CP11 Full Access */
#endif

  //Move initialization data int RAM
  uint32_t *dst = (uint32_t *) & __sdata;
  uint32_t *src = (uint32_t *) & __etext;

  while (dst < (uint32_t *) & __edata)
    *(dst++) = *(src++);

  int irq;

  for (irq = 0; irq < IRQ_HANDLER_POOL; irq++)
    pool[irq].next = &pool[irq + 1];
  pool[IRQ_HANDLER_POOL - 1].next = NULL;
  for (irq = 0; irq < ISR_SIZE; irq++)
    handlers[irq] = NULL;

  main();
}

static int irqRef = 0;
int amIInIRQ(void)
{
  return (irqRef != 0);
}

static void __attribute__ ((no_instrument_function, interrupt, optimize("-O3"))) IRQHandler(void)
{
  irqRef++;

  irqHandler_t *handle = handlers[getIrqNum()];

  while (handle)
    {
      handle->handler();
      handle = handle->next;
    }
  irqRef--;
}

static void __attribute__ ((no_instrument_function, interrupt)) Unused_Handler(void)
{
}


int registerIRQ(int irq, void (*handler) (void))
{
  taskENTER_CRITICAL();
  if (!poolFree)
    {
      taskEXIT_CRITICAL();
      dprintf(LL_ERROR, "IRQ handlers pool depleted.");
      return -1;
    }
  irq += 16;
  if (irq >= ISR_SIZE)
    {
      taskEXIT_CRITICAL();
      dprintf(LL_ERROR, "IRQ handler index invalid.");
      return -2;
    }
  irqHandler_t *handle = poolFree;

  poolFree = poolFree->next;

  handle->next = handlers[irq];
  handlers[irq] = handle;
  handle->handler = handler;
  taskEXIT_CRITICAL();
  return 0;
}
