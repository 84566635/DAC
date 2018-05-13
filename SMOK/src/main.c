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
#include <piny_ADC_FPGA.h>
#include <FPGA_Bitstream.h>
#include <FPGA_Bitstream_size.h>

void delay_us(uint32_t delay);
void delay_100ns(uint32_t delay);
static void configure_FPGA(void);
void Kalibracja_init(void);
void Kalibracja_(void);

///////////////////////////////////////////////////
/*-----------------------------------------------------------*/
//-------------------------------------------------------------------------
static void configure_FPGA(void)
{
    uint8_t i;
    uint32_t j;
    uint8_t Data;

	PROGRAMN_L_L;
	PROGRAMN_R_L;

	mdelay(10);

	PROGRAMN_L_H;
	PROGRAMN_R_H;

	mdelay(10);


    for(j=0; j<FPGA_Bitstream_size[0]; j++)
        {
    	Data = FPGA_Bitstream[j];

        for(i=0; i<8; i++)
            {
            if(Data & 0x80)
            	{
            	FPGA_L_DI_H;
            	FPGA_R_DI_H;
            	}
            else
            	{
            	FPGA_L_DI_L;
            	FPGA_R_DI_L;
            	}

            delay_100ns(1);

            FPGA_L_CCLK_H;
            FPGA_R_CCLK_H;

            delay_100ns(1);

        	FPGA_L_CCLK_L;
        	FPGA_R_CCLK_L;

        	delay_100ns(1);

            Data <<= 1;
            };
        }


}
//-------------------------------------------------------------------------
void systemInit(void)
{
  hwBoardInit();

  void messageProcessorInit(void);
  messageProcessorInit();

  /* Initialize tasks */
  LEDInit();
  ADCInit();
  configure_FPGA();
  Kalibracja_init();
  spiHelperInit();
  CommunicatorInit();
  HWCommInit();
  debugInit();
  (void)DACVOLUME_Init();
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
