
#include <project.h>
#include <common.h>
#include <string.h>

#include STMINCP()
#include "FreeRTOS.h"
#include "task.h"

#include <stdint.h>
#include <bqueue.h>
#include <communicator.h>
#include <messageProcessor.h>
#include <messageProcessorSMOK.h>
#include <power_proc.h>
#include <uit_monitor.h>
#include <led.h>
#include <watchdog.h>
#include <i2c.h>
#include <cfg.h>
#include <adc.h>

// Numery komunikatoe 0xB0
#define Uwe_MsgCode      0x21          //Okresowe przesłanie wartości napięcia	
#define Uwy_MsgCode      0x22          //Okresowe przesłanie wartości napięcia	
#define Iavg_MsgCode     0x23          //Okresowe przesłanie wartości średniego 
#define Imax_MsgCode     0x24          //Okresowe przesłanie wartości maksymalnego 
#define Temp_MsgCode     0x30          //Okresowa wartość temperatury	
#define Sharc_NC_MsgCode 0xA1          //Brak połączenia z Sharc
#define PWM_NC_MsgCode   0xA2          //Brak połączenia z PWM	
#define Sharc_C_MsgCode  0xA3          //Połączenie z Sharc
#define PWM_C_MsgCode    0xA4          //Połączenie z PWM	
#define FET_ERR_MsgCode  0xB1          //Błąd temperatury FET	
#define AIR_ERR_MsgCode  0xB2          //Błąd temperatury AIR
#define Uwe_Low_MsgCode  0xB3          //Błąd za niskiego napięcia Uwe	
#define Uwe_High_MsgCode 0xB4          //Błąd zbyt wysokiego napięcia Uwe
#define IL_High_MsgCode  0xB5          //Błąd zbyt wyskiego prądu IL	
#define IH_High_MsgCode  0xB6          //Błąd zbyt wyskiego prądu IH
#define Uwy_High_MsgCode 0xB8          //Błąd zbyt wysokiego napięcia Uwy

#define SNDROUND(arg_value) ((arg_value)<0)?0:(((arg_value)>>8)&0xFF), ((arg_value)<0)?0:(((arg_value)>>0)&0xFF)
static smokState_e smokState = Smok_NC;
static int UIT_MsgRound    = 0;

//=====================================================
//   P O W E R _ I N I T
//=====================================================

//--------------------------------------------------------------
//  C O N T R O L   T E M P E R A T U R E  & C U R R E N T
//--------------------------------------------------------------

// Wartosci pradu/napiecia wyliczane w odpowiedniej funkcji
int temp1, temp2;
#define TEMP2C(arg) ((arg)>>7)
void power_SHD_ERR_P(uint8_t errorCode)
{
  //  LEDset(0, ~0, 0);

  //Switch on LED's depends from errorCode
  switch(errorCode)
    {
    case TEMP_FET_ERROR:
      //      LEDset(0, LED_RED| LED_GREEN, 1);
      break;
    case TEMP_AIR_ERROR:
      //      LEDset(0, LED_RED| LED_GREEN | LED_YELLOW, 1);
      break;
    case WAVE_OK1_ERROR:
    case WAVE_OK2_ERROR:
      //      LEDset(0, LED_RED| LED_GREEN | LED_BLUE, 1);
      break;
    case UWE_H_ERROR:
      //      LEDset(0, LED_RED| LED_ORANGE, 1);
      break;
    case UWE_L_ERROR:
      //      LEDset(0, LED_RED| LED_ORANGE | LED_YELLOW, 1);
      break;
    case IL_ERROR:
      //      LEDset(0, LED_RED| LED_ORANGE | LED_BLUE, 1);
      break;
    case IH_ERROR:
      //      LEDset(0, LED_RED| LED_ORANGE | LED_BLUE, 1);
      break;
    case DATAGRAM_CONN_LOST:
      //      LEDset(0, LED_RED| LED_BLUE, 1);
      break;
    case CONN_LOST:
      //      LEDset(0, LED_RED| LED_BLUE | LED_YELLOW, 1);
      break;
    }
}

void wdogFail(void)
{
  if(!(cfg.disable_alerts&0x02))
    {
      powerOFF1();
      mdelay(200);
      powerOFF2();
    }
  while(1);
}

int isOn(void)
{
  return (smokState == Smok_ON);
}

static void getVoltages(void)
{
  //clear measures
  CLRADC(Uwe_val);
  CLRADC(Uwy_val);

  //Wait until at least one measure is done
  int retry = 100;
  while((Uwe_val_smpl == 0 || Uwy_val_smpl == 0) && retry--)
    mdelay(1);

  if(retry <= 0)
    dprintf(LL_ERROR, "Uwy or Uwe measure failed!!!\n");
}

int Check_power1(void)
{
  int ok = 1;
  getVoltages();
  //Check Uve
  dprintf(LL_INFO, "Check Uwe_val %d (alarm %d) \n", Uwe_val, cfg.uwe_alarm);
  if(Uwe_val >= cfg.uwe_alarm)
    {
      dprintf(LL_WARNING, "Uwe_val to high %d >= %d \n", Uwe_val, cfg.uwe_alarm);
      power_SHD_ERR_P(UWE_H_ERROR);
      if(!(cfg.disable_alerts&0x04))
        {
          sendStatus(Uwe_High_MsgCode, SNDROUND(Uwe_val));
          ok = 0;
          //SmokChangeState(Smok_OFF);
        }
    }
  return ok;
}

int Check_power2(void)
{
  int ok = 1;
  getVoltages();
  //Check Uvy
  dprintf(LL_INFO, "Check Uwy_val %d (alarm %d) \n", Uwy_val, cfg.uwy_alarm);
  if(Uwy_val >= cfg.uwy_alarm)
    {
      dprintf(LL_WARNING, "Uwy_val to high %d >= %d \n", Uwy_val, cfg.uwy_alarm);
      power_SHD_ERR_P(UWY_H_ERROR);
      if(!(cfg.disable_alerts&0x08))
        {
          sendStatus(Uwy_High_MsgCode, SNDROUND(Uwy_val));
          ok = 0;
          //SmokChangeState(Smok_OFF);
        }
    }
  return ok;
}

int Check_power3(void)
{
  int ok = 1;
  if(!(pwmStatus()&0x01))
    {
      dprintf(LL_WARNING, "PWM_STAT error \n");
      if(!(cfg.disable_alerts&0x10))
        {
          sendStatus(0xB9, 0, 0);
          ok = 0;
          SmokChangeState(Smok_OFF, "PWM_STAT error - poweron");
          SmokLEDsState(0, -1, SET_OTHER, -1);
        }
    }
  return ok;
}

int Check_power4(void)
{
  int ok = 1;
  getVoltages();

  dprintf(LL_INFO, "Check Uwy=10%WUwe Uwy = %dmV Uwe = %dmV\n", Uwy_val, Uwe_val);

  //Check Uve/Uwy
  if(Uwy_val < ((Uwe_val*7)/100) || Uwy_val > ((Uwe_val*13)/100))
    {
      dprintf(LL_WARNING, "Uwy_val(%dmV) outside of allowed range compared to Uwe(%dmV)  0.07*Uwe<Uwy<0.13*Uwe \n", Uwy_val, Uwe_val);
      if(!(cfg.disable_alerts&0x01))
        {
          sendStatus(0xB5, SNDROUND(Uwy_val));
          ok = 0;
          SmokChangeState(Smok_OFF, "Uwy vs Uwe failed");
          SmokLEDsState(0, -1, SET_OTHER, -1);
        }
    }
  return ok;
}

static uint32_t nextPossibleSend = 0;
static void Check_UIT_Alarms(void)
{
  if(nextPossibleSend > xTaskGetTickCount())
    return;

  if(smokState == Smok_ON)
    {
      int sent = 0;;
      int pwm = pwmStatus();
      if(!(pwm&(1<<0)))
        {
          dprintf(LL_WARNING, "PWM_STAT error \n");
          if(!(cfg.disable_alerts&0x10))
            {
              sendStatus(0xB9, 0, 0);
              SmokChangeState(Smok_OFF, "PWM_STAT error - alert");
              SmokLEDsState(0, -1, SET_OTHER, -1);
            }
          sent = 1;
        }
      if(!(pwm&(1<<1)))
        {
          dprintf(LL_WARNING, "PWM PB3 error \n");
          if(!(cfg.disable_alerts&0x20))
            {
              sendStatus(0xBA, 0xA, 0);
              SmokChangeState(Smok_OFF, "PWM PB3 error");
              SmokLEDsState(0, -1, SET_OTHER, -1);
            }

          sent = 1;
        }
      if(!(pwm&(1<<2)))
        {
          dprintf(LL_WARNING, "PWM PB4 error \n");
          if(!(cfg.disable_alerts&0x40))
            {
              sendStatus(0xBA, 0xB, 0);
              SmokChangeState(Smok_OFF, "PWM PB4 error");
              SmokLEDsState(0, -1, SET_OTHER, -1);
            }
          sent = 1;
        }
      if(!(pwm&(1<<3)))
        {
          dprintf(LL_WARNING, "PWM PC14 error \n");
          if(!(cfg.disable_alerts&0x80))
            {
              sendStatus(0xBB, 0xA, 0);
              SmokChangeState(Smok_OFF, "PWM PC14 error");
              SmokLEDsState(0, -1, SET_OTHER, -1);
            }
          sent = 1;
        }
      if(!(pwm&(1<<4)))
        {
          dprintf(LL_WARNING, "PWM PC15 error \n");
          if(!(cfg.disable_alerts&0x100))
            {
              sendStatus(0xBB, 0xB, 0);
              SmokChangeState(Smok_OFF, "PWM PC15 error");
              SmokLEDsState(0, -1, SET_OTHER, -1);
            }
          sent = 1;
        }
      if(!(pwm&(1<<5)))
        {
          dprintf(LL_WARNING, "PWM PE13 error \n");
          if(!(cfg.disable_alerts&0x2000))
            {
              sendStatus(0xBB, 0xC, 0);
              SmokChangeState(Smok_OFF, "PWM PE13 error");
              SmokLEDsState(0, -1, SET_OTHER, -1);
            }
          sent = 1;
        }

      /* //Check Uvy */
      /* if(Uwy_val >= cfg.uwy_alarm) */
      /*   { */
      /*     dprintf(LL_WARNING, "Uwy_val to high %d >= %d \n", Uwy_val, cfg.uwy_alarm); */
      /*     sendStatus(Uwy_High_MsgCode, ((Uwy_val>>8) & 0xFF),(Uwy_val & 0xFF)); */
      /*     power_SHD_ERR_P(UWY_H_ERROR); */
      /*     if(!cfg.disable_alerts) SmokChangeState(Smok_OFF); */
      /*     sent = 1; */
      /*   } */

      //Check IL
      if(meaniL >= cfg.imax_alarm)
        {
          dprintf(LL_WARNING, "meaniL to high %d >= %d \n", meaniL, cfg.imax_alarm);
          power_SHD_ERR_P(IL_ERROR);
          if(!(cfg.disable_alerts&0x200))
            {
              sendStatusLong(IL_High_MsgCode, SNDROUND(meaniL), SNDROUND(sMaxvL));
              SmokChangeState(Smok_OFF, "meaniL to high");
              SmokLEDsState(0, -1, SET_OTHER, -1);
            }
          sent = 1;
        }

      //Check Temperature AIR
      if(TEMP2C(temp1) >= cfg.t_air_max)
        {
          dprintf(LL_WARNING, "tAir to high %d.%d >= %d.%d \n", TEMP2C(temp1)/2, 5*(TEMP2C(temp1)%2), cfg.t_air_max/2, 5*(cfg.t_air_max%2));
          power_SHD_ERR_P(TEMP_AIR_ERROR);
          if(!(cfg.disable_alerts&0x400))
            {
              sendStatus(AIR_ERR_MsgCode, TEMP2C(temp1), 0);
              SmokChangeState(Smok_OFF, "tAir to high");
              SmokLEDsState(0, -1, SET_TEMP, -1);
            }
          sent = 1;
        }

      //Check temperature FET
      if(TEMP2C(temp2) >= cfg.t_fet_max)
        {
          dprintf(LL_WARNING, "tFET to high %d.%d >= %d.%d \n", TEMP2C(temp2)/2, 5*(TEMP2C(temp2)%2), cfg.t_fet_max/2, 5*(cfg.t_fet_max%2));
          power_SHD_ERR_P(TEMP_FET_ERROR);
          if(!(cfg.disable_alerts&0x800))
            {
              sendStatus(FET_ERR_MsgCode, TEMP2C(temp2),0);
              SmokChangeState(Smok_OFF, "tFET to high");
              SmokLEDsState(0, -1, SET_TEMP, -1);
            }
          sent = 1;
        }
      if(sent)
        {
          //Next send slot calculate
          nextPossibleSend = xTaskGetTickCount() + cfg.ka_period;
        }
    }
}

void uitTimeout(portNum_t portNum)
{
  if(smokState == Smok_ON)
    {
      if(portNum == PER1_PORT)
        {
          sendStatus(Sharc_NC_MsgCode, 0,0);
          //          LEDblink(0, LED_GREEN, 1, 200, 1, 1, 200, 0);
          dprintf(LL_WARNING, "Sharc did not respond on keepalive message\n");
        }
      if(portNum == PER2_PORT)
        {
          sendStatus(PWM_NC_MsgCode, 0,0);
          //          LEDblink(0, LED_ORANGE, 1, 200, 1, 1, 200, 0);
          dprintf(LL_WARNING, "PWM did not respond on keepalive message\n");
        }
    }
}

void uitOk(portNum_t portNum)
{
  if(portNum == PER1_PORT)
    {
      sendStatus(Sharc_C_MsgCode, 0,0);
    }

  if(portNum == PER2_PORT)
    {
      sendStatus(PWM_C_MsgCode, 0,0);
    }
}

#define SENDSTS(arg_mask, arg_sendSeq, arg_clrSeq)      \
  {                                                     \
    uint8_t mask = (arg_mask)?0x00:0x01;                \
    if( (arg_mask) & (1<<UIT_MsgRound) ) mask = 3;      \
    if(mask & 2) {arg_sendSeq;}                         \
    if(mask & 1){arg_clrSeq;}                           \
  }

static int nextTimestamp = 0;
static int nextTempMeasure = 0;
static void UIT_Monitor(int adcTime)
{

  int diff = nextTempMeasure - adcTime;
  if(diff < 0 || diff > 100000)
    {
      nextTempMeasure = adcTime + cfg.temp_time;
      uint8_t buffer[2] = {0};
      if(cfg.term1Addr >= 0 && i2cTransfer(I2C1, cfg.term1Addr, buffer, 1, buffer, 2)>=2)
        temp1 = ((int)(buffer[0])<<8) | buffer[1];
      buffer[0] = 0;
      buffer[1] = 0;
      if(cfg.term2Addr >= 0 && i2cTransfer(I2C1, cfg.term2Addr, buffer, 1, buffer, 2)>=2)
        temp2 = ((int)(buffer[0])<<8) | buffer[1];
    }

  Check_UIT_Alarms();

  {
    if(Uwy_k > cfg.uwyRange && kSensADC == 1) sensADCnew = 0;
    if(Uwy_k <= cfg.uwyRange && kSensADC == 0) sensADCnew = 1;
    CLRADC(Uwy_k);
  }

  diff = nextTimestamp - adcTime;
#define IDX ((smokState == Smok_ON)?1:0)
  if(diff < 0 || diff > 10000)
    {
      nextTimestamp = adcTime + cfg.msgRoundTime[IDX];

      //Next round
      UIT_MsgRound = (UIT_MsgRound + 1)%(cfg.msgRoundMax[IDX]+1);

      //Send actual current, voltage and temperature output values
      // -------  Uwe -----------
      SENDSTS(cfg.msgUwe[IDX],
              sendStatus(Uwe_MsgCode, SNDROUND(Uwe_val)),
              CLRADC(Uwe_val));

      // -------  Uwy -----------
      SENDSTS(cfg.msgUwy[IDX],
              sendStatus(Uwy_MsgCode, SNDROUND(Uwy_val)),
              CLRADC(Uwy_val);
             );

      // -------  Iavg -----------
      SENDSTS(cfg.msgIavg[IDX],
              sendStatusLong(Iavg_MsgCode, SNDROUND(meaniL), SNDROUND(UwyI)),
      {
        CLRADC(UweI);
        CLRADC(UwyI);
        CLRADC(meaniL);
        CLRADC(meaniH);
      });

      // -------  Imax -----------
      SENDSTS(cfg.msgImax[IDX],
              sendStatusLong(Imax_MsgCode, SNDROUND(sMaxiL), SNDROUND(sMaxvL)); ,
              sMaxiL = 0;
             );

      // -------  Temperature AIR & FET -----------
      SENDSTS(cfg.msgTemp[IDX],
              sendStatus(Temp_MsgCode, TEMP2C(temp2), TEMP2C(temp1)); ,
              /**/
             );
    }
}

static int ADCtime = 0;
void communicatorADCMeasure(int samplesNum, int sampleSensADC,
                            int V1accu, int V2accu,
                            int I1max, int V2I1max, int I1accu, int I2accu)
{
  //Function is called every ADC_WINDOW [ms]
  ADCtime += ADC_WINDOW;

  UIT_Monitor(ADCtime);
}

void poweroffSeq(void)
{
  //reset PLL
  pll_set(0, 1);

  //disable keepalivest
  kaCCEnable(0);
  //turn off RST_AMP
  powerOFF1();
  //Send off info to PWM
  sendPWMoffMsg();
  mdelay(100);
  //disable power_on
  powerOFF2();
  mdelay(500);

  //Disable PWM_STAT
  powerOFF3();
  Check_power1();

}

typedef int (*changeState_t)(smokState_e oldState, smokState_e newState);

static int stateChangeInvalid(smokState_e oldState, smokState_e newState)
{
  dprintf(LL_WARNING, "Invalid state requested.\n");
  return -1;
}

static int stateChangeAlreadyThere(smokState_e oldState, smokState_e newState)
{
  dprintf(LL_WARNING, "Already in requested state.\n");
  return 1;
}

static int stateChangeNC2OFF(smokState_e oldState, smokState_e newState)
{
  //  LEDset(0, ~0, 0);
  //  LEDset(0, LED_GREEN, 1);
  dprintf(LL_INFO, "NC->OFF\n");
  return 0;
}

static int stateChangeON2OFF(smokState_e oldState, smokState_e newState)
{
  dprintf(LL_INFO, "ON->OFF\n");
  poweroffSeq();
  return 0;
}

static int stateChangeOFF2ON(smokState_e oldState, smokState_e newState)
{
  dprintf(LL_INFO, "OFF->ON\n");
  kaCCEnable(1);
  return 0;
}

static int stateChangeOFF2NC(smokState_e oldState, smokState_e newState)
{
  dprintf(LL_INFO, "OFF->NC\n");
  return 0;
}

static int stateChangeON2NC(smokState_e oldState, smokState_e newState)
{
  dprintf(LL_INFO, "ON->NC\n");
  poweroffSeq();
  return 0;
}


static const changeState_t changeStateMatrix[SmokStateNum/*oldState*/][SmokStateNum/*newState*/] =
{
  [Smok_NC] =
  {
    [Smok_NC]  = stateChangeAlreadyThere,
    [Smok_OFF] = stateChangeNC2OFF,
    [Smok_ON]  = stateChangeInvalid,
  },
  [Smok_OFF] =
  {
    [Smok_NC]  = stateChangeOFF2NC,
    [Smok_OFF] = stateChangeAlreadyThere,
    [Smok_ON]  = stateChangeOFF2ON,
  },
  [Smok_ON] =
  {
    [Smok_NC]  = stateChangeON2NC,
    [Smok_OFF] = stateChangeON2OFF,
    [Smok_ON]  = stateChangeAlreadyThere,
  },
};

int SmokChangeState(smokState_e newState, const char *reason)
{
  int ret = 0;

  //Call a proper action for the state change
  ret = changeStateMatrix[smokState][newState](smokState, newState);

  //Set new state
  if(ret >= 0)
    {
      smokState = newState;
      UIT_MsgRound = 0;
      dprintf(LL_INFO, "power state change reason: %s\n", reason);
    }

  return ret;
}


void SmokLEDsState(int on, int conn, smokErrType_t err, int hd)
{
  int ledsMaskOn = 0;
  int ledsMaskOff = 0;
#define SET_DIODE(arg_diode) {ledsMaskOff &= ~(arg_diode);ledsMaskOn  |= (arg_diode);}
#define CLR_DIODE(arg_diode) {ledsMaskOn  &= ~(arg_diode);ledsMaskOff |= (arg_diode);}

  if(conn == 0)
    {
      CLR_DIODE(LED_WHITE);
    }

  if(conn == 1)
    {
      SET_DIODE(LED_WHITE);
    }

  //Turn on
  if(on == 1)
    {
      CLR_DIODE(LED_RED | LED_YELLOW);
      if(hd == 1)
        SET_DIODE(LED_BLUE);
      if(hd == 0)
        CLR_DIODE(LED_BLUE);
    }

  //Turn off
  if(on == 0)
    {
      SET_DIODE(LED_YELLOW);
      switch(err)
        {
        case SET_NONE:
          CLR_DIODE(LED_BLUE);
          break;
        case SET_CONN:
          SET_DIODE(LED_RED);
          CLR_DIODE(LED_WHITE | LED_BLUE);
          break;
        case SET_TEMP:
          SET_DIODE(LED_RED);
          CLR_DIODE(LED_BLUE);
          break;
        case SET_OTHER:
          SET_DIODE(LED_RED | LED_BLUE | LED_WHITE);
          break;
        }
    }

  LEDset(0, ledsMaskOn, ledsMaskOff);
}
