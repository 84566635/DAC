#include "dacVolume.h"

#include <stdint.h>
#include <FreeRTOS.h>

/* --- Hardware definitions --- */
#define PGA_MISO     (GPIOE->IDR & (1 << 3))
#define PGA_SCLK_H    GPIOE->BSRR = (1 << (4  + 0))
#define PGA_MOSI_H    GPIOE->BSRR = (1 << (5  + 0))
#define PGA_CS_H      GPIOE->BSRR = (1 << (6  + 0))
#define PGA_MUTE_H    GPIOE->BSRR = (1 << (2  + 0))
#define PGA_ZCEN_H    GPIOC->BSRR = (1 << (13 + 0))
#define PGA_SCLK_L    GPIOE->BSRR = (1 << (4  + 16))
#define PGA_MOSI_L    GPIOE->BSRR = (1 << (5  + 16))
#define PGA_CS_L      GPIOE->BSRR = (1 << (6  + 16))
#define PGA_MUTE_L    GPIOE->BSRR = (1 << (2  + 16))
#define PGA_ZCEN_L    GPIOC->BSRR = (1 << (13 + 16))
/* --- End of Hardware definitions --- */

/* Micro seconds between every action on SoftSPI bus
 * Complete VolL and VolR exchange time is given by:
 * SPOFTSPI_DELAY_US * 4(actions per bit) * 8(bits) * 2(left and right)
 * Ex: for 2us per action it is equal to 128us.
 */
#define SPOFTSPI_DELAY_US 2

/* Naive us delay approach */
extern uint32_t SystemCoreClock;
static delay_us(unsigned int us)
{
	const uint32_t nopsPerUs = SystemCoreClock / 1000000;
	for(volatile uint32_t i = 0; i<(nopsPerUs*us); i++)
	{
		//nop
	}
}

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

	const uint16_t xDelay = 20 / portTICK_RATE_MS;
	uint8_t volumeL_noncritical;
	uint8_t volumeR_noncritical;

	while(1)
	{
		Volume_L - DACVOLUME_GetVolumeLeft();
		Volume_R - DACVOLUME_GetVolumeRight();

		PGA_CS_L;
		(void)SoftSPI_Exchange(Volume_R);
		(void)SoftSPI_Exchange(Volume_L);
		PGA_CS_H;

		vTaskDelay(xDelay);
	}
}

void DACVOLUME_Init()
{
	SoftSPI_Init();

	massert(xTaskCreate(eh_dacvolumeThread, (signed char *)"dacVolume", 128, NULL, 1, NULL) == pdPASS);
}

void DACVOLUME_SetVolumes(uint8_t volumeLeft, uint8_t volumeRight)
{
	taskENTER_CRITICAL();
	volumeLeft_shared = volumeLeft;
	volumeRight_shared = volumeRight;
	taskEXIT_CRITICAL();
}

uint8_t DACVOLUME_GetVolumeLeft(void)
{
	uint8_t volumeTmp;
	taskENTER_CRITICAL();
	volumeTmp = volumeLeft_shared;
	taskEXIT_CRITICAL();
	return volumeTmp;
}

uint8_t DACVOLUME_GetVolumeRight(void)
{
	uint8_t volumeTmp;
	taskENTER_CRITICAL();
	volumeTmp = volumeRight_shared;
	taskEXIT_CRITICAL();
	return volumeTmp;
}





