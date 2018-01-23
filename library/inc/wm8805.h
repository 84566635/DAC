#ifndef WM8805_H_INCLUDED
#define WM8805_H_INCLUDED


#define _94_3104_shl_22 (94310400ULL<<22ULL)
#define _98_304_shl_22  (98304000ULL<<22ULL)
#ifndef WM_XTAL_FREQ
# define WM_XTAL_FREQ 12000000ULL
#endif

#if WM_XTAL_FREQ >= 24000000ULL
#define PRESCALER 1
#else
#define PRESCALER 0
#endif
#define WM88XX_PLLN_94_3104MHz (uint32_t)(((PRESCALER+1)*_94_3104_shl_22/(WM_XTAL_FREQ))>>22ULL)
#define WM88XX_PLLK_94_3104MHz (uint32_t)(((PRESCALER+1)*_94_3104_shl_22/(WM_XTAL_FREQ))-((WM88XX_PLLN_94_3104MHz)<<22ULL))

#define WM88XX_PLLN_98_304MHz (uint32_t)(((PRESCALER+1)*_98_304_shl_22/(WM_XTAL_FREQ))>>22ULL)
#define WM88XX_PLLK_98_304MHz (uint32_t)(((PRESCALER+1)*_98_304_shl_22/(WM_XTAL_FREQ))-((WM88XX_PLLN_98_304MHz)<<22ULL))

//WM8805 registers
#define WM8805_REG_RSTDEVID1 0x00
#define WM8805_REG_DEVID2    0x01
#define WM8805_REG_DEVREV    0x02

#define WM8805_REG_PWRDN     0x1E

#define PWRDN_PLLPD          0x01
#define PWRDN_SPDIFRXPD      0x02
#define PWRDN_SPDIFTXPD      0x04
#define PWRDN_OSCPD          0x08
#define PWRDN_AIFPD          0x10
#define PWRDN_TRIOP          0x20
#define PWRDN_ENABLE_ALL     0x00

#define WM8805_REG_SPDRX1    0x1D

#define SPDRX1_SPD_192K_EN   0x80
#define SPDRX1_WL_MASK       0x40

#define SPDRX1_IRQ_STAT_REG  0x00
#define SPDRX1_CH_STAT_REG1  0x01
#define SPDRX1_CH_STAT_REG2  0x02
#define SPDRX1_CH_STAT_REG3  0x03
#define SPDRX1_CH_STAT_REG4  0x04
#define SPDRX1_CH_STAT_REG5  0x05
#define SPDRX1_SPDIF_STAT    0x06

#define SPDRX1_CONT          0x08

#define WM8805_REG_PLL1      0x03
#define WM8805_REG_PLL2      0x04
#define WM8805_REG_PLL3      0x05
#define WM8805_REG_PLL4      0x06

#define PLL4_PRESCALE        0x10

#define WM8805_REG_PLL5      0x07

#define PLL5_FRACEN          0x04

#define WM8805_REG_PLL6      0x08

#define PLL6_RX0             0x00
#define PLL6_RX1             0x01
#define PLL6_RX2             0x02
#define PLL6_RX3             0x03
#define PLL6_RX4             0x04
#define PLL6_RX5             0x05
#define PLL6_RX6             0x06
#define PLL6_RX7             0x07

#define WM8805_REG_AIFRX     0x1C

#define AIFRX_AIFMS          0x40
#define AIFRX_SYNC_OFF       0x80

#define WM8805_REG_SPDMODE   0x09

#define SPDMODE_CMOS_INPUT       0x00
#define SPDMODE_COMPARATOR_INPUT 0xFF

#define WM8805_REG_IRQMASK    0x0A

#define IRQMASK_UPD_UNLOCK    0x01
#define IRQMASK_INT_INVALID   0x02
#define IRQMASK_INT_CSUD      0x04
#define IRQMASK_INT_TRANS_ERR 0x08
#define IRQMASK_UPD_NON_AUDIO 0x10
#define IRQMASK_UPD_CPY_N     0x20
#define IRQMASK_UPD_DEEMPH    0x40
#define IRQMASK_UPD_REC_FREQ  0x80

#define WM8805_REG_IRQSTAT    0x0B

#define IRQSTAT_UPD_UNLOCK    0x01
#define IRQSTAT_INT_INVALID   0x02
#define IRQSTAT_INT_CSUD      0x04
#define IRQSTAT_INT_TRANS_ERR 0x08
#define IRQSTAT_UPD_NON_AUDIO 0x10
#define IRQSTAT_UPD_CPY_N     0x20
#define IRQSTAT_UPD_DEEMPH    0x40
#define IRQSTAT_UPD_REC_FREQ  0x80

#define WM8805_REG_SPDIF     0x0C
#define WM8805_REG_RXCH1     0x0D
#define WM8805_REG_RXCH2     0x0E
#define WM8805_REG_RXCH3     0x0F
#define WM8805_REG_RXCH4     0x10
#define WM8805_REG_RXCH5     0x11

#define WM8805_REG_AIFTX     0x1B

#define AIFTX_DSP_MODE       0x03
#define AIFTX_I2S_MODE       0x02
#define AIFTX_LJUST_MODE     0x01
#define AIFTX_RJUST_MODE     0x00

#define AIFTX_WL_24BITS_A    0x0C //? A check in datasheet
#define AIFTX_WL_24BITS_B    0x08 //?
#define AIFTX_WL_20BITS      0x04
#define AIFTX_WL_16BITS      0x00

#define AIFTX_BCLK_INVERT    0x10
#define AIFTX_LRP_INVERT     0x20

#define WM8805_REG_AIFRX     0x1C

#define AIFRX_DSP_MODE       0x03
#define AIFRX_I2S_MODE       0x02
#define AIFRX_LJUST_MODE     0x01
#define AIFRX_RJUST_MODE     0x00

#define AIFRX_WL_24BITS_A    0x0C //? A check in datasheet
#define AIFRX_WL_24BITS_B    0x08 //?
#define AIFRX_WL_20BITS      0x04
#define AIFRX_WL_16BITS      0x00

#define AIFRX_BCLK_INVERT    0x10
#define AIFRX_LRP_INVERT     0x20

#define WM_HW_MODE 0
#define WM_SW_MODE 1


#define WM8805_REG_SPDTX1    0x12
#define WM8805_REG_SPDTX2    0x13
#define WM8805_REG_SPDTX3    0x14
#define WM8805_REG_SPDTX4    0x15
#define WM8805_REG_SPDTX5    0x16


#define SET_PIN(arg_Pin, state_arg) GPIO_WriteBit(arg_Pin.GPIOx, arg_Pin.GPIO_Pin_x, state_arg)

#define ENABLE_WM(wm880x_arg)  {volatile int delay = 20; while(delay-- > 10);SET_PIN( (&(wm880x[wm880x_arg]))->CS_Pin, 0);while(delay--);}
#define DISABLE_WM(wm880x_arg) {volatile int delay = 20; while(delay-- > 10);SET_PIN( (&(wm880x[wm880x_arg]))->CS_Pin, 1);while(delay--);}

#define RST_ACTIVE 0
#define RST_INACTIVE 1
#define RESET_WM5(arg_reSET) GPIO_WriteBit( GPIOC, GPIO_Pin_14, arg_reSET)
#define RESET_WM4(arg_reSET) GPIO_WriteBit( GPIOC, GPIO_Pin_13, arg_reSET)

#define WM_READ   0x80
#define WM_WRITE  0x00

//Defines for WM88XXSetReceiverChannel and WM88XXSelectStatusRegisterToRead
#define WM88XX_RECV_CHANNEL0 0x00
#define WM88XX_RECV_CHANNEL1 0x01
#define WM88XX_RECV_CHANNEL2 0x02
#define WM88XX_RECV_CHANNEL3 0x03
#define WM88XX_RECV_CHANNEL4 0x04
#define WM88XX_RECV_CHANNEL5 0x05
#define WM88XX_RECV_CHANNEL6 0x06
#define WM88XX_RECV_CHANNEL7 0x07

//Defines for WM8805 mode
#define WM88XX_MASTER_MODE   AIFRX_AIFMS
#define WM88XX_SLAVE_MODE    0x00

#define WM88XX_TX_INPUT_AIF   0x40
#define WM88XX_TX_INPUT_SPDIF 0x00

#define WM_CHIP_CNT 4

typedef enum
{
  WM8805_1 = 0,
  WM8805_2,
  WM8804_1,
  WM8804_2
} spdifChip_t;

typedef struct
{
  GPIO_TypeDef *GPIOx;
  uint16_t GPIO_Pin_x;
} pin_t;

typedef struct
{
  pin_t CS_Pin;
} wm880x_t;

typedef enum
{
  WM880x_I2S,
  WM880x_DSP
} wm880x_audioMode_t;

void WM88XXInit(void);
int WM88XXWriteRegister(spdifChip_t spdifChip, int reg, int val);
int WM88XXReadRegister(spdifChip_t spdifChip, int reg);
void WM88XXSoftwareReset(spdifChip_t spdifChip);
void WM88XXReadID(spdifChip_t spdifChip, char *tab);
void WM88XXSetPLLMul(spdifChip_t spdifChip);
void WM88XXSetReceiverChannel(spdifChip_t spdifChip, int channel);
void WM88XXSetTransmitterInput(spdifChip_t spdifChip, int input);
void WM88XXSetTransmitterWL(spdifChip_t spdifChip, int val);
void WM88XXSetAIFRXMode(spdifChip_t spdifChip, char val);
void WM88XXSetAIFTXMode(spdifChip_t spdifChip, char val);
void WM88XXSetMode(spdifChip_t spdifChip, char val);
void WM88XXSetInterruptMask(spdifChip_t spdifChip, char mask);
int WM88XXIrqProc(spdifChip_t spdifChip);
int WM88XXReturnCurrentFreq(spdifChip_t spdifChip);
int WM88XX_AudioInterfaceSetState(spdifChip_t spdifChip, uint8_t state);
void WM8804setResolution(spdifChip_t wmChip, int resolution);
int WM88XXGetLock(spdifChip_t spdifChip);
void WM8805DBG(int *bufSize, char *bufPtr);

#endif
