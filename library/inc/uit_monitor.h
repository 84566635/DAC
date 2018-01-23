typedef enum
{
  Smok_NC,
  Smok_OFF,
  Smok_ON,
  SmokStateNum,
} smokState_e;
int SmokChangeState(smokState_e newState, const char *reason);
void uitTimeout(portNum_t portNum);
void uitOk(portNum_t portNum);

#define TEMP_AIR_ERROR  0x01
#define TEMP_FET_ERROR  0x02
#define IL_ERROR        0x11
#define IH_ERROR        0x12
#define WAVE_OK1_ERROR  0x21
#define WAVE_OK2_ERROR  0x22
#define UWE_H_ERROR     0x31
#define UWE_L_ERROR     0x32
#define MAIN_LOOP_ERROR 0x51
#define SHARC_COM_ERROR 0x81
#define PWM_COM_ERROR   0x82
#define DATAGRAM_CONN_LOST 0x91
#define CONN_LOST       0x92

#define UWY_H_ERROR     0x31

void power_SHD_ERR_P(uint8_t errorCode);

void ampEnable(int enable);
void powerON1(void);
void powerON2(void);
void powerON3(void);
void powerON4(void);
void powerOFF1(void);
void powerOFF2(void);
void powerOFF3(void);
void sendPWMoffMsg(void);
int Check_power1(void);
int Check_power2(void);
int Check_power3(void);
int Check_power4(void);
uint8_t pwmStatus(void);
void poweroffSeq(void);

int isOn(void);

typedef enum
{
  SET_NONE,
  SET_CONN,
  SET_TEMP,
  SET_OTHER,
} smokErrType_t;

void SmokLEDsState(int on, int conn, smokErrType_t err, int hd);
