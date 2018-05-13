#include "dacVolume.h"

#include <stm32f4xx.h>
#include <stm32f4xx_gpio.h> /* because of buggy stm32f4xx_hwInit.h */
#include <stm32f4xx_rcc.h> /* because of buggy stm32f4xx_hwInit.h */

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h> /* because of buggy stm32f4xx_hwInit.h */

#include <stm32f4xx_hwInit.h>

#include "common.h" /* massert */
#include <piny_ADC_FPGA.h>

void Kalibracja_init(void);
void Kalibracja_(void);
void delay_us(uint32_t delay);


/*
 * Micro seconds between every action on SoftSPI bus
 * Complete VolL and VolR exchange time is given by:
 * SPOFTSPI_DELAY_US * 4(actions per bit) * 8(bits) * 2(left and right)
 * ex. for 2us per action complete exchange is equal to 128us.
 */
#define SPOFTSPI_DELAY_US 2

/*
 * Period in milliseconds between every volume exchange
 */
#define DACVOLUME_PERIOD_MS 20



/*
 * Exchange data on SPI bus.
 * Function drives clock and MISO, MOSI lines to exchange data o bus.
 * Function does not drive CS pin.
 * Bus timing can be set by SPOFTSPI_DELAY_US macro.
 * @param inputData 8-bit word to send to bus
 * @return output 8-bit word from bus
 */
static uint8_t SoftSPI_Exchange(uint8_t inputData)
{
	uint8_t i;
	uint8_t outputData = 0;

	for(i = 0; i < 8; i++)
	{
		/* Set BIT on BUS */
		delay_us(SPOFTSPI_DELAY_US);
		if(inputData & 0x80)
		{
			PGA_MOSI_H;
		}
		else
		{
			PGA_MOSI_L;
		}
		inputData <<= 1;
		delay_us(SPOFTSPI_DELAY_US);

		PGA_SCLK_H;
		delay_us(SPOFTSPI_DELAY_US);

		/*Read BIT form BUS */
		outputData <<= 1;
		if (PGA_MISO != 0)
		{
			outputData |= 0x01;
		}

		delay_us(SPOFTSPI_DELAY_US);
		PGA_SCLK_L;
	}
	return outputData;
}
//-------------------------------------------------------------------------
static uint8_t SPI_PGA_Wr_Rd(uint8_t OutputData)
{
    uint8_t i;
    uint8_t InputData = 0;

    delay_us(100);

    for(i=0; i<8; i++)
        {
        if(OutputData & 0x80)
        	PGA_MOSI_H;
        else
        	PGA_MOSI_L;

        delay_us(100);

        PGA_SCLK_H;

        delay_us(100);

        InputData <<= 1;

        if (PGA_MISO != 0)
        	InputData |= 0x01;

        OutputData <<= 1;

    	PGA_SCLK_L;

        delay_us(100);
        };

    return InputData;
}
//-------------------------------------------------------------------------
static uint8_t volumeLeft_shared;
static uint8_t volumeRight_shared;

static void eh_dacvolumeThread(void* pvParameters)
{
	(void)pvParameters;

	const uint16_t msDelay = DACVOLUME_PERIOD_MS / portTICK_RATE_MS;
	uint8_t Volume_L;
	uint8_t Volume_R;

	Kalibracja_();
	Kalibracja_();

	while(1)
	{
		taskENTER_CRITICAL();
		Volume_L = volumeLeft_shared;
		Volume_R = volumeRight_shared;
		taskEXIT_CRITICAL();

		PGA_CS_L;
		(void)SPI_PGA_Wr_Rd(Volume_R);
		(void)SPI_PGA_Wr_Rd(Volume_L);
		PGA_CS_H;

		vTaskDelay(msDelay);
	}
}

int DACVOLUME_Init()
{
	//if(cfg.centralDeviceMode == DEVMODE_DAC)
	if(DEVMODE_DAC == DEVMODE_DAC)
	{
		massert(xTaskCreate(eh_dacvolumeThread, (signed char *)"dacVolume", 128, NULL, 1, NULL) == pdPASS);
		return 0;
	}
	return -1;
}

void DACVOLUME_SetVolumes(uint8_t volumeLeft, uint8_t volumeRight)
{
	taskENTER_CRITICAL();
	volumeLeft_shared = volumeLeft;
	volumeRight_shared = volumeRight;
	taskEXIT_CRITICAL();
}
