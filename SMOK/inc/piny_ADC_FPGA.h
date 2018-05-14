#include <stm32f4xx.h>

//===============
#define uC_IO_0_H     GPIOE->BSRRL = (1 << (10 + 0))
#define uC_IO_1_H     GPIOE->BSRRL = (1 << (11 + 0))
#define uC_IO_2_H     GPIOE->BSRRL = (1 << (12 + 0))
#define uC_IO_3_H     GPIOE->BSRRL = (1 << (13 + 0))
#define uC_IO_4_H     GPIOE->BSRRL = (1 << (14 + 0))
#define uC_IO_5_H     GPIOE->BSRRL = (1 << (15 + 0))

#define uC_IO_0_L     GPIOE->BSRRH = (1 << (10 + 0))
#define uC_IO_1_L     GPIOE->BSRRH = (1 << (11 + 0))
#define uC_IO_2_L     GPIOE->BSRRH = (1 << (12 + 0))
#define uC_IO_3_L     GPIOE->BSRRH = (1 << (13 + 0))
#define uC_IO_4_L     GPIOE->BSRRH = (1 << (14 + 0))
#define uC_IO_5_L     GPIOE->BSRRH = (1 << (15 + 0))
//===============
//===============
#define FREE_IO_0_H   GPIOD->BSRRL = (1 << (0  + 0))
#define FREE_IO_1_H   GPIOD->BSRRL = (1 << (8  + 0))
#define FREE_IO_2_H   GPIOD->BSRRL = (1 << (9  + 0))
#define FREE_IO_3_H   GPIOD->BSRRL = (1 << (10 + 0))
#define FREE_IO_4_H   GPIOD->BSRRL = (1 << (14 + 0))
#define FREE_IO_5_H   GPIOD->BSRRL = (1 << (15 + 0))

#define FREE_IO_0_L   GPIOD->BSRRH = (1 << (0  + 0))
#define FREE_IO_1_L   GPIOD->BSRRH = (1 << (8  + 0))
#define FREE_IO_2_L   GPIOD->BSRRH = (1 << (9  + 0))
#define FREE_IO_3_L   GPIOD->BSRRH = (1 << (10 + 0))
#define FREE_IO_4_L   GPIOD->BSRRH = (1 << (14 + 0))
#define FREE_IO_5_L   GPIOD->BSRRH = (1 << (15 + 0))
//===============



//===============
#define ADC_SCLK_H    GPIOG->BSRRL = (1 << (4  + 0))
#define ADC_MOSI_H    GPIOG->BSRRL = (1 << (3  + 0))
#define ADC_CS_H      GPIOG->BSRRL = (1 << (2  + 0))
#define uC_IO_5b_H    GPIOG->BSRRL = (1 << (7  + 0))

#define ADC_SCLK_L    GPIOG->BSRRH = (1 << (4  + 0))
#define ADC_MOSI_L    GPIOG->BSRRH = (1 << (3  + 0))
#define ADC_CS_L      GPIOG->BSRRH = (1 << (2  + 0))
#define uC_IO_5b_L    GPIOG->BSRRH = (1 << (7  + 0))

#define ADC_MISO      (GPIOG->IDR & (1 << 5))
#define ADC_DRDY      (GPIOG->IDR & (1 << 6))
//===============
//===============
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

#define PGA_MISO      (GPIOE->IDR & (1 << 3))
//===============
//===============
#define FPGA_L_CCLK_H   GPIOF->BSRRL = (1 << (0  + 0))
#define FPGA_L_CCLK2_H  GPIOF->BSRRL = (1 << (1  + 0))
#define FPGA_L_DI_H     GPIOF->BSRRL = (1 << (2  + 0))
#define FPGA_L_CS_H     GPIOF->BSRRL = (1 << (11 + 0))
#define PROGRAMN_L_H    GPIOF->BSRRL = (1 << (15 + 0))

#define FPGA_L_CCLK_L   GPIOF->BSRRH = (1 << (0  + 0))
#define FPGA_L_CCLK2_L  GPIOF->BSRRH = (1 << (1  + 0))
#define FPGA_L_DI_L     GPIOF->BSRRH = (1 << (2  + 0))
#define FPGA_L_CS_L     GPIOF->BSRRH = (1 << (11 + 0))
#define PROGRAMN_L_L    GPIOF->BSRRH = (1 << (15 + 0))

#define FPGA_DO_L       (GPIOF->IDR & (1 << 12))
#define DONE_L          (GPIOF->IDR & (1 << 13))
#define INITN_L         (GPIOF->IDR & (1 << 14))
//===============
//===============
#define FPGA_R_CCLK_H   GPIOF->BSRRL = (1 << (10 + 0))
#define FPGA_R_CCLK2_H  GPIOF->BSRRL = (1 << (9  + 0))
#define FPGA_R_DI_H     GPIOF->BSRRL = (1 << (8  + 0))
#define FPGA_R_CS_H     GPIOF->BSRRL = (1 << (3  + 0))
#define PROGRAMN_R_H    GPIOF->BSRRL = (1 << (7  + 0))

#define FPGA_R_CCLK_L   GPIOF->BSRRH = (1 << (10 + 0))
#define FPGA_R_CCLK2_L  GPIOF->BSRRH = (1 << (9  + 0))
#define FPGA_R_DI_L     GPIOF->BSRRH = (1 << (8  + 0))
#define FPGA_R_CS_L     GPIOF->BSRRH = (1 << (3  + 0))
#define PROGRAMN_R_L    GPIOF->BSRRH = (1 << (7  + 0))

#define FPGA_DO_R       (GPIOF->IDR & (1 << 4))
#define DONE_R          (GPIOF->IDR & (1 << 5))
#define INITN_R         (GPIOF->IDR & (1 << 6))
//===============

#define RST_L_L    GPIOF->BSRRH = (1 << (12 + 0))
#define RST_R_L    GPIOF->BSRRH = (1 << (4  + 0))
#define RST_L_H    GPIOF->BSRRL = (1 << (12 + 0))
#define RST_R_H    GPIOF->BSRRL = (1 << (4  + 0))

#define ADC_LA     0x9C
#define ADC_LB     0x8C
#define ADC_LC     0x3C

#define ADC_RA     0x2C
#define ADC_RB     0x1C
#define ADC_RC     0x0C

#define ADC_RX    ADC_RB



#define Kalibracja 2    //0 - uproszczona ci¹g³a, 2- pe³na i praca, 1 - pomiary napiêæ zasilania,
                        //3 - brak kalibracji i praca
