void ADCInit(void);
void ADC_startConversion(uint16_t *buffer, int size);
void ADC_stopConversion(void);
void ADC_hwInit(void);
extern int sensADCnew;
extern int kSensADC;

extern int bufferNum;

#define  CLRADC(arg_val)  \
  {                       \
    taskENTER_CRITICAL(); \
    arg_val##_data = 0;   \
    arg_val##_smpl = 0;   \
    taskEXIT_CRITICAL();  \
  }


extern int Uwe_val, Uwy_val, sMaxiL, sMaxvL, meaniL, UwyI, meaniH, UweI, Uwy_k;
extern unsigned long long Uwe_val_data, Uwy_val_data, meaniL_data, UwyI_data, meaniH_data, UweI_data, Uwy_k_data;
extern unsigned int Uwe_val_smpl, Uwy_val_smpl, meaniL_smpl, UwyI_smpl, meaniH_smpl, UweI_smpl, Uwy_k_smpl;
