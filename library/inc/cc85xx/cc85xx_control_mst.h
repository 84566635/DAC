
#ifndef CC85XX_CONTROL_MST_H_
#define CC85XX_CONTROL_MST_H_

/* #ifdef CC85XX_MODE */
/* #if CC85XX_MODE != MASTER */
/* #error "Host may use only one type of cc85xx Devices: Slaves or Masters" */
/* #endif */
/* #else */
/* #define CC85XX_MODE = MASTER */
/* #endif */


// CC85XX states
#define CC85XX_STATE_OFF            0
#define CC85XX_STATE_INACTIVE       1




void test_master(void);

void chipRadioEnable(uint8_t state);

EHIF_CMD_NWM_GET_STATUS_MASTER_DATA_T getStatus(void);

void enablePairing(void);
void disablePairing(void);
EHIF_CMD_VC_SET_VOLUME_PARAM_T setVolume(void);
void rfChannels(uint32_t channelsMask);



#endif
