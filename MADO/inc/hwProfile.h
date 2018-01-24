#ifndef HW_PROFILE_H_
#define HW_PROFILE_H_


#include <common.h>

//hwprofile:
#define OLINUXINO_DMA_CHANNEL 4
#define OLINUXINO_DMAx DMA2
#define OLINUXINO_DMAx_STREAMx DMA2_Stream0
#define OLINUXINO_DMA_IT_TCIFx DMA_IT_TCIF0
#define OLINUXINO_DMA_FLAG_TCIFx DMA_FLAG_TCIF0
#define OLINUXINO_DMA_STREAM_NUM 0

#define SPIx_AUDIO_STREAM 4 // SPIx

/* #define OLINUXINO_ENABLE_PIN 15 */
/* #define OLINUXINO_ENABLE_GPIO E */
//hwprofile end



#define SAI_BlockA_DMA2_StreamX DMA2_Stream1
#define BlockA_IRQn DMA2_Stream1_IRQn
#define DMA_SAI_BlockA_Channel DMA_Channel_0
#define SAI1_Block_A_DataReg ((uint32_t) &(SAI1_Block_A->DR) )


#define DMA_IT_TCIF_BlockA DMA_IT_TCIF1
#define DMA_IT_TCIF_BlockA DMA_IT_TCIF1
#define DMA_IT_TEIF_BlockA DMA_IT_TEIF1
#define DMA_IT_FEIF_BlockA DMA_IT_FEIF1
#define DMA_IT_DMEIF_BlockA DMA_IT_DMEIF1
#define DMA_FLAG_TCIF_BlockA DMA_FLAG_TCIF1
#define DMA_FLAG_TEIF_BlockA DMA_FLAG_TEIF1
#define DMA_FLAG_FEIF_BlockA DMA_FLAG_FEIF1
#define DMA_FLAG_DMEIF_BlockA DMA_FLAG_DMEIF1

#define SAI_BlockB_DMA2_StreamX DMA2_Stream5
#define BlockB_IRQn DMA2_Stream5_IRQn
#define DMA_SAI_BlockB_Channel DMA_Channel_0
#define SAI1_Block_B_DataReg ((uint32_t) &(SAI1_Block_B->DR) )

#define DMA_IT_TCIF_BlockB DMA_IT_TCIF5
#define DMA_IT_TEIF_BlockB DMA_IT_TEIF5
#define DMA_IT_FEIF_BlockB DMA_IT_FEIF5
#define DMA_IT_DMEIF_BlockB DMA_IT_DMEIF5

#define DMA_FLAG_TCIF_BlockB DMA_FLAG_TCIF5
#define DMA_FLAG_TEIF_BlockB DMA_FLAG_TEIF5
#define DMA_FLAG_FEIF_BlockB DMA_FLAG_FEIF5
#define DMA_FLAG_DMEIF_BlockB DMA_FLAG_DMEIF5


#define INPUT_I2S_SPIx SPI3

typedef enum
{
  I2S3_DMA,
} saiBlocks;


//Clock MHz = N/Q
//2* WCLK = Clock (MHz)/256

//should be 44,1 kHz, closest possible is 44,2 kHz!

#define PLLN_44K 271   // 192 - 432    gives PLLI2SN MHz on VCO
#define PLLQ_44K 12    //  2-15
#define PLLR_44K 7     // 2 - 7

#define PLLN_48K 344   // 192 - 432    gives PLLI2SN MHz on VCO
#define PLLQ_48K 14    //  2-15
#define PLLR_48K 7     // 2 - 7

#define PLLN_88K 271   // 192 - 432    gives PLLN MHz on VCO
#define PLLQ_88K 6    //  2-15
#define PLLR_88K 7     // 2 - 7

#define PLLN_96K 344   // 192 - 432    gives PLLN MHz on VCO
#define PLLQ_96K 7    //  2-15
#define PLLR_96K 7     // 2 - 7

#define DECODE_N(arg) (((uint32_t)(arg)>>16)&0xFFFF)
#define DECODE_Q(arg) (((uint32_t)(arg)>>8)&0xFF)
#define DECODE_D(arg) (((uint32_t)(arg))&0xFF)
#define _N_IDX 0
#define _Q_IDX 1
#define _R_IDX 2


// RADIO_1 source select defs:
#define RADIO1_SRC_WM8805_2 0
#define RADIO1_SRC_SAI_A 1


/* I2S peripheral configuration defines */
#define I2S_INPUT_GPIO_AF              GPIO_AF_SPI3
#define I2S_INPUT_GPIO_CLOCK           (RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOA)

#define I2S_INPUT_WS_GPIO               GPIOA
#define I2S_INPUT_SCK_GPIO              GPIOC
#define I2S_INPUT_SD_GPIO               GPIOC
// #define I2S_INPUT_MCK_GPIO             GPIOC
#define I2S_INPUT_WS_PIN               GPIO_Pin_4
#define I2S_INPUT_SCK_PIN              GPIO_Pin_10
#define I2S_INPUT_SD_PIN               GPIO_Pin_12
// #define I2S_INPUT_MCK_PIN              GPIO_Pin_7
#define I2S_INPUT_WS_PINSRC            GPIO_PinSource4
#define I2S_INPUT_SCK_PINSRC           GPIO_PinSource10
#define I2S_INPUT_SD_PINSRC            GPIO_PinSource12
// #define I2S_INPUT_MCK_PINSRC           GPIO_PinSource7



#endif
