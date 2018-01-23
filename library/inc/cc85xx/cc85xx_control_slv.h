
#ifndef CC85XX_CONTROL_SLV_H_
#define CC85XX_CONTROL_SLV_H_

/* #ifdef CC85XX_MODE */
/* #if CC85XX_MODE != SLAVE */
/* #error "Host may use only one type of cc85xx Devices: Slaves or Masters" */
/* #endif */
/* #else */
/* #define CC85XX_MODE = SLAVE */
/* #endif */

// CC85XX states
#define CC85XX_STATE_OFF            0
#define CC85XX_STATE_ALONE          1
#define CC85XX_STATE_PAIRING        2
// #define CC85XX_STATE_ACTIVE         3



void test_slave(void);

void disconnectFromMaster(void);
// test for master present and connects
uint8_t doJoin(uint32_t masterAddress, uint32_t masterManID);


EHIF_CMD_NWM_DO_SCAN_DATA_T doScan(uint32_t timeout_ms);

uint8_t CheckNetwork(void);

EHIF_CMD_NWM_GET_STATUS_SLAVE_DATA_T getStatus(void);

void saveAdrNonVolatile(void);

uint32_t getLastDisconnected(void);

void activateChannels(uint8_t *channels);

EHIF_CMD_DI_GET_DEVICE_INFO_DATA_T getNetworkID(uint32_t masterID, uint32_t timeout_ms);
EHIF_CMD_VC_GET_VOLUME_PARAM_T getVolume(void);



#endif
