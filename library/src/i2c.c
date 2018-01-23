#include <project.h>
#include <common.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include STMINCP()
#include <semphr.h>
#include STMINCS(_rcc)
#include STMINCS(_gpio)
#include STMINCS(_i2c)
#include STMINCS(_hwInit)

#include <stdint.h>

#include <i2c.h>

#define FLAG_TIMEOUT             ((uint32_t)2)
#define LONG_TIMEOUT             ((uint32_t)(10 * FLAG_TIMEOUT))

int i2cInit(I2C_TypeDef *i2c)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  /* Enable I2C GPIO clocks */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB , ENABLE);

  if(i2c == I2C1)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
  else
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;

  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  /* Connect pins to I2C peripheral */
  if(i2c == I2C1)
    {
      GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_I2C1);
      GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_I2C1);
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
    }
  else
    {
      GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_I2C2);
      GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_I2C2);
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);
    }

  I2C_InitTypeDef I2C_InitStructure;


  I2C_DeInit(i2c);
  I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
  I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
  I2C_InitStructure.I2C_OwnAddress1 = 0x33;
  I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
  I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
  I2C_InitStructure.I2C_ClockSpeed = 100000;
  /* Enable the I2C peripheral */
  I2C_Cmd(i2c, ENABLE);
  I2C_Init(i2c, &I2C_InitStructure);
  return 0;
}



#define ERR_EXIT_IF_TMOUT(arg_stop)		\
if ((timeout--) == 0)				\
  {						\
    if(arg_stop)I2C_GenerateSTOP(i2c, ENABLE);	\
    return -(__LINE__);				\
  }						\
 else						\
   mdelay(1);

int i2cWrite(I2C_TypeDef *i2c, uint8_t addr, uint8_t *data, uint16_t size)
{
  uint32_t timeout = 0;
  /*!< While the bus is busy */
  timeout = LONG_TIMEOUT;
  while (I2C_GetFlagStatus(i2c, I2C_FLAG_BUSY))
    ERR_EXIT_IF_TMOUT(0);

  /* Start the config sequence */
  I2C_GenerateSTART(i2c, ENABLE);

  /* Test on EV5 and clear it */
  timeout = FLAG_TIMEOUT;
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_MODE_SELECT))
    ERR_EXIT_IF_TMOUT(1);

  /* Transmit the slave address and enable writing operation */
  I2C_Send7bitAddress(i2c, addr<<1, I2C_Direction_Transmitter);

  /* Test on EV6 and clear it */
  timeout = FLAG_TIMEOUT;
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
    ERR_EXIT_IF_TMOUT(1);

  int i;
  for(i = 0; i < size; i++)
    {
      /* Transmit the first address for write operation */
      I2C_SendData(i2c, data[i]);

      /* Test on EV8 and clear it */
      timeout = FLAG_TIMEOUT;
      while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTING))
        ERR_EXIT_IF_TMOUT(1);

    }
  /*!< Wait till all data have been physically transferred on the bus */
  timeout = LONG_TIMEOUT;
  while (!I2C_GetFlagStatus(i2c, I2C_FLAG_BTF))
    ERR_EXIT_IF_TMOUT(1);

  /* End the configuration sequence */
  I2C_GenerateSTOP(i2c, ENABLE);
  return size;
}

int i2cRead(I2C_TypeDef *i2c, uint8_t addr, uint8_t *data, uint16_t size)
{
  uint32_t timeout = 0;

  /*!< While the bus is busy */
  timeout = LONG_TIMEOUT;
  while (I2C_GetFlagStatus(i2c, I2C_FLAG_BUSY))
    ERR_EXIT_IF_TMOUT(0);

  /*!< Send START condition a second time */
  I2C_GenerateSTART(i2c, ENABLE);

  /*!< Test on EV5 and clear it (cleared by reading SR1 then writing to DR) */
  timeout = FLAG_TIMEOUT;
  while (!I2C_CheckEvent(i2c, I2C_EVENT_MASTER_MODE_SELECT))
    ERR_EXIT_IF_TMOUT(1);

  /*!< Send TEMP_SENSOR address for read */
  I2C_Send7bitAddress(i2c, addr<<1, I2C_Direction_Receiver);

  /* Wait on ADDR flag to be set (ADDR is still not cleared at this level */
  timeout = FLAG_TIMEOUT;
  while (I2C_GetFlagStatus(i2c, I2C_FLAG_ADDR) == RESET)
    ERR_EXIT_IF_TMOUT(1);

  /* Clear ADDR register by reading SR1 then SR2 register (SR1 has already been read) */
  (void)i2c->SR2;

  int i;
  for(i = 0; i < size; i++)
    {
      if(i == size -1)
        {
          //Last byte
          /*!< Disable Acknowledgment */
          I2C_AcknowledgeConfig(i2c, DISABLE);
          /*!< Send STOP Condition */
          I2C_GenerateSTOP(i2c, ENABLE);
        }
      /* Wait for the byte to be received */
      timeout = FLAG_TIMEOUT;
      while (I2C_GetFlagStatus(i2c, I2C_FLAG_RXNE) == RESET)
        ERR_EXIT_IF_TMOUT(1);

      /*!< Read the byte received from the TEMP_SENSOR */
      data[i] = I2C_ReceiveData(i2c);
    }

  /*!< Re-Enable Acknowledgment to be ready for another reception */
  I2C_AcknowledgeConfig(i2c, ENABLE);

  /* Clear AF flag for next communication */
  I2C_ClearFlag(i2c, I2C_FLAG_AF);

  return size;
}

int i2cTransfer(I2C_TypeDef *i2c, uint8_t addr, uint8_t *txData, uint16_t txSize, uint8_t *rxData, uint16_t rxSize)
{
  int ret = 0;
  if(txSize && txData)
    ret = i2cWrite(i2c, addr, txData, txSize);
  if(rxSize && rxData && ret > 0)
    ret = i2cRead(i2c, addr, rxData, rxSize);
  return ret;
}
