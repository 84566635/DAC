#ifndef CC85XX_SPI_CONF_H_
#define CC85XX_SPI_CONF_H_

#warning "Zautomatyzowac ...PIN, ...SRC"

// #define ___PIN(arg)    #arg

#define __PIN(arg)    GPIO_Pin_##arg
#define __PIN_SRC(arg)  GPIO_PinSource##arg


#define FUNCTION_NAME(name) #name
#define TEST_FUNC_NAME  FUNCTION_NAME(test_func)


#define CC85XX_SPI_GPIO   GPIOC
#define CC85XX_SPI      SPI3

#define CC85XX_SPI_SCK    GPIO_PinSource10

#define CC85XX_SPI_MOSI_SRC GPIO_PinSource12
#define CC85XX_SPI_MOSI_PIN GPIO_Pin_12
#define CC85XX_SPI_MOSI_PIN_N 12

#define CC85XX_SPI_MISO_SRC   GPIO_PinSource11
#define CC85XX_SPI_MISO_PIN   GPIO_Pin_11

#define CC85XX_SPI_CS_SRC GPIO_PinSource10
#define CC85XX_SPI_CS_PIN GPIO_Pin_10
#define CC85XX_SPI_CS_GPIO  GPIOA

#define CC85XX_RSTn_GPIO  GPIOA
#define CC85XX_RSTn_PIN   GPIO_Pin_12

#define CC85XX_IRQn_GPIO  GPIOA
#define CC85XX_IRQn_SRC   GPIO_PinSource8
#define CC85XX_IRQn_PIN   GPIO_Pin_8



#define CC85XX_GPIO_AF    GPIO_AF_SPI3


/// Specify the MCU clock speed used in number if MHz (e.g. 16 for 16 MHz) (for timing events on SPI)
#define EHIF_MCU_SPEED_IN_MHZ               180



#endif