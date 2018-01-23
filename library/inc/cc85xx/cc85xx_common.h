
#ifndef CC85XX_COMMON_H_
#define CC85XX_COMMON_H_

#include <cc85xx_ehif_utils.h>
#include <cc85xx_ehif_defs.h>
#include <cc85xx_ehif_basic_op.h>
#include <cc85xx_ehif_hal_board.h>
#include <cc85xx_ehif_hal_mcu.h>
#include <cc85xx_ehif_cmd_exec.h>


typedef struct
{
  uint32_t devID;
  uint32_t manID;
  uint32_t prodID;
  //uint8_t datagramChState;
} device_t;


typedef struct
{
  uint32_t devID;
  uint32_t manID;
  uint32_t prodID;

  uint16_t audioConnState;
} remoteDev_t;      //or master



// CC85XX states
#define CC85XX_STATE_ACTIVE         3

#define POWER_STATUS_OFFSET 9

//power
#define CC85XX_POWER_MASK           0x7 << POWER_STATUS_OFFSET
#define CC85XX_POWER_STATE_ACTIVE             0x5 << POWER_STATUS_OFFSET
#define CC85XX_POWER_STATE_LP               0x4 << POWER_STATUS_OFFSET
#define CC85XX_POWER_STATE_LOCAL_STANDBY    0x3 << POWER_STATUS_OFFSET
#define CC85XX_POWER_STATE_NETWORK_STANDBY    0x2 << POWER_STATUS_OFFSET
#define CC85XX_POWER_STATE_OFF              0x0 << POWER_STATUS_OFFSET



//Global definitions
#define MAX_PAYLOAD 32
#define PAYLOAD_PLUS_ADR MAX_PAYLOAD + 5

// interrupt active level
#define ACTIVE_HIGH   1
#define ACTIVE_LOW    0


int  reset_powerActive(void);
void powerOff(void);

//
EHIF_CMD_DI_GET_DEVICE_INFO_DATA_T getDeviceInfo(void);

//
EHIF_CMD_DI_GET_CHIP_INFO_DATA_T getChipInfo(void);

//
uint8_t cc85xx_Check_RX(void);

//
int queueRX(uint32_t *address, uint8_t *ret_data, uint8_t *reset);

//
int queueTX(remoteDev_t *dev, uint16_t dataLength, uint8_t *data, uint8_t reset);

//
void clearFlags( uint8_t mask);

//
void extiISRHandler(void);

//
void setIntEvents( uint8_t mask);

#define WAIT_BUSY while(!(ehifGetStatus()&BV_EHIF_STAT_CMD_REQ_RDY))   EHIF_DELAY_MS(1);


// channel definitions
#define FRONT_PRIMARY_LEFT	 	0
#define FRONT_PRIMARY_RIGHT 	1
#define REAR_SECONDARY_LEFT 	2
#define REAR_SECONDARY_RIGHT 	3
#define FRONT_CENTER 			4
#define SUBWOOFER 				5
#define SIDE_LEFT 				6
#define SIDE_RIGHT 				7
#define USER_DEFINED_0 			8
#define USER_DEFINED_1 			9
#define INPUT_LEFT 				10
#define INPUT_RIGHT 			11
#define MICROPHONE_0 			12
#define MICROPHONE_1 			13
#define MICROPHONE_2 			14
#define MICROPHONE_3 			15

#define MAX_SLOT_NUM 16

#endif
