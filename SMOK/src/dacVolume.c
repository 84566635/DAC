#include "dacVolume.h"

#include <stm32f4xx.h>
#include <stm32f4xx_gpio.h> /* because of buggy stm32f4xx_hwInit.h */
#include <stm32f4xx_rcc.h> /* because of buggy stm32f4xx_hwInit.h */

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h> /* because of buggy stm32f4xx_hwInit.h */

#include <stm32f4xx_hwInit.h>

#include "common.h" /* massert */

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
 * Naive us delay approach
 * Depends on external value SystemCoreClock,
 * which is assumed as externally provided and consistent for every frame.
 */
static void delay_us(unsigned int us)
{
	extern uint32_t SystemCoreClock;
	const uint32_t nopsPerUs = SystemCoreClock / 1000000;
	for(volatile uint32_t i = 0; i<(nopsPerUs*us); i++)
	{
		//nop
	}
}

/* --- Hardware SoftSPI GPIOS definitions --- */
#define PGA_MISO     (GPIOE->IDR & (1 << 3))
#define PGA_SCLK_H    GPIOE->BSRRL = (1 << (4  + 0))
#define PGA_MOSI_H    GPIOE->BSRRL = (1 << (5  + 0))
#define PGA_CS_H      GPIOE->BSRRL = (1 << (6  + 0))
#define PGA_MUTE_H    GPIOE->BSRRL = (1 << (2  + 0))
#define PGA_ZCEN_H    GPIOC->BSRRL = (1 << (13 + 0))
#define PGA_SCLK_L    GPIOE->BSRRH = (1 << (4  + 0))
#define PGA_MOSI_L    GPIOE->BSRRH = (1 << (5  + 0))
#define PGA_CS_L      GPIOE->BSRRH = (1 << (6  + 0))
#define PGA_MUTE_L    GPIOE->BSRRH = (1 << (2  + 0))
#define PGA_ZCEN_L    GPIOC->BSRRH = (1 << (13 + 0))

/*
 * Initialize MCU general I/O mode to drive software SPI,
 * and set it to default states.
 */
static void SoftSPI_Init(void)
{

	GPIO_INIT(E, 4,  OUT, NOPULL);   /* SoftSPI Clock (SCLK) */
	GPIO_INIT(E, 5,  OUT, NOPULL);   /* SoftSPI MOSI */
	GPIO_INIT(E, 6,  OUT, NOPULL);   /* SoftSPI Chip Select (CS) */
	GPIO_INIT(E, 3,  IN,  UP);       /* SoftSPI MISO */
	GPIO_INIT(E, 2,  OUT, NOPULL);   /* PGA_MUTE */
	GPIO_INIT(C, 13, OUT, NOPULL);   /* PGA_ZCEN */

	PGA_CS_H;
	PGA_ZCEN_L;
	PGA_MUTE_H;
}

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

static uint8_t volumeLeft_shared;
static uint8_t volumeRight_shared;

static void eh_dacvolumeThread(void* pvParameters)
{
	(void)pvParameters;

	const uint16_t msDelay = DACVOLUME_PERIOD_MS / portTICK_RATE_MS;
	uint8_t Volume_L;
	uint8_t Volume_R;

	while(1)
	{
		taskENTER_CRITICAL();
		Volume_L = volumeLeft_shared;
		Volume_R = volumeRight_shared;
		taskEXIT_CRITICAL();

		PGA_CS_L;
		(void)SoftSPI_Exchange(Volume_R);
		(void)SoftSPI_Exchange(Volume_L);
		PGA_CS_H;

		vTaskDelay(msDelay);
	}
}

int DACVOLUME_Init()
{
	if(cfg.centralDeviceMode == DEVMODE_DAC)
	{
		SoftSPI_Init();
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
