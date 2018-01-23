
typedef enum
{
  CONF_TIMEOUT,
  NEG_CONFIRMATION,
  MA_SMOK,
  TP_MA,
  PER_GEN,
  MA_GEN,
  SMOK_GEN,
  SMOK_PER,
} types_e;

#define CB_RET_ACK       0x01
#define CB_RET_nACK      0x02

struct comm_s;
typedef struct comm_s comm_t;
struct comm_s
{
  int code;
  int size;
  int type;
  int nackCode;
  int spk_flags;
  int size_RX;
  int reserved3;
  int reserved4;
  int timeout;
  int blocked;
  int received[PORTS_NUM];
  int sent[PORTS_NUM];
  int (*callback)(portNum_t portNum, comm_t *comm, bBuffer_t *buf, uint8_t *forwardMask);
  int (*timeoutCallback)(portNum_t portNum, comm_t *comm);
  bBuffer_t *buffers;
} ;
#define COMM_TBL_SET(arg_code, arg_nackCode, arg_type, arg_spkFlags, arg_timeout, arg_callback, arg_timeoutCallback) \
  {									\
    .code = 0x##arg_code,						\
      .nackCode = 0x##arg_nackCode,					\
      .size = sizeof(msg_##arg_code##_t),				\
      .size_RX = sizeof(msg_##arg_code##r_t),				\
      .type = arg_type,							\
      .blocked = 0,							\
      .spk_flags = arg_spkFlags,					\
      .timeout = arg_timeout,						\
      .callback = arg_callback,						\
      .timeoutCallback = arg_timeoutCallback,				\
      .buffers = NULL, .received = {0},}

extern comm_t MSG_TYPES_TAB[MSG_TYPES_QTY];

void Timeout_Data(portNum_t portNum, char *buffer, int bufferSize);
comm_t *msgTab(uint8_t code);
void communicatorPeriodicPrivate(void);
int expectedMsgSize(portNum_t portNum, comm_t *msgType);
void msgGenSend(bBuffer_t *msg);

#define u8 uint8_t

#define DEF_MSG_TABLE(arg_code, a1, size)                                      typedef struct {u8 kod;u8 a1[size];}__attribute__((packed))msg_##arg_code##_t
#define DEF_MSG_1(arg_code)                                                    typedef struct {u8 kod;}__attribute__((packed))msg_##arg_code##_t
#define DEF_MSG_5(arg_code,a1,a2,a3,a4)                                        typedef struct {u8 kod;u8 a1;u8 a2;u8 a3;u8 a4;}__attribute__((packed))msg_##arg_code##_t
#define DEF_MSG_6(arg_code,a1,a2,a3,a4,a5)                                     typedef struct {u8 kod;u8 a1;u8 a2;u8 a3;u8 a4;u8 a5;}__attribute__((packed))msg_##arg_code##_t
#define DEF_MSG_7(arg_code,a1,a2,a3,a4,a5,a6)                                  typedef struct {u8 kod;u8 a1;u8 a2;u8 a3;u8 a4;u8 a5;u8 a6;}__attribute__((packed))msg_##arg_code##_t
#define DEF_MSG_8(arg_code,a1,a2,a3,a4,a5,a6,a7)                               typedef struct {u8 kod;u8 a1;u8 a2;u8 a3;u8 a4;u8 a5;u8 a6;u8 a7;}__attribute__((packed))msg_##arg_code##_t
#define DEF_MSG_9(arg_code,a1,a2,a3,a4,a5,a6,a7,a8)                            typedef struct {u8 kod;u8 a1;u8 a2;u8 a3;u8 a4;u8 a5;u8 a6;u8 a7;u8 a8;}__attribute__((packed))msg_##arg_code##_t
#define DEF_MSG_10(arg_code,a1,a2,a3,a4,a5,a6,a7,a8,a9)                        typedef struct {u8 kod;u8 a1;u8 a2;u8 a3;u8 a4;u8 a5;u8 a6;u8 a7;u8 a8;u8 a9;}__attribute__((packed))msg_##arg_code##_t
#define DEF_MSG_11(arg_code,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10)                    typedef struct {u8 kod;u8 a1;u8 a2;u8 a3;u8 a4;u8 a5;u8 a6;u8 a7;u8 a8;u8 a9;u8 a10;}__attribute__((packed))msg_##arg_code##_t
#define DEF_MSG_12(arg_code,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11)                typedef struct {u8 kod;u8 a1;u8 a2;u8 a3;u8 a4;u8 a5;u8 a6;u8 a7;u8 a8;u8 a9;u8 a10;u8 a11;}__attribute__((packed))msg_##arg_code##_t
#define DEF_MSG_13(arg_code,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12)            typedef struct {u8 kod;u8 a1;u8 a2;u8 a3;u8 a4;u8 a5;u8 a6;u8 a7;u8 a8;u8 a9;u8 a10;u8 a11;u8 a12;}__attribute__((packed))msg_##arg_code##_t
#define DEF_MSG_14(arg_code,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13)        typedef struct {u8 kod;u8 a1;u8 a2;u8 a3;u8 a4;u8 a5;u8 a6;u8 a7;u8 a8;u8 a9;u8 a10;u8 a11;u8 a12;u8 a13;}__attribute__((packed))msg_##arg_code##_t
#define DEF_MSG_15(arg_code,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14)    typedef struct {u8 kod;u8 a1;u8 a2;u8 a3;u8 a4;u8 a5;u8 a6;u8 a7;u8 a8;u8 a9;u8 a10;u8 a11;u8 a12;u8 a13;u8 a14;}__attribute__((packed))msg_##arg_code##_t
#define DEF_MSG_16(arg_code,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15)typedef struct {u8 kod;u8 a1;u8 a2;u8 a3;u8 a4;u8 a5;u8 a6;u8 a7;u8 a8;u8 a9;u8 a10;u8 a11;u8 a12;u8 a13;u8 a14;u8 a15;}__attribute__((packed))msg_##arg_code##_t

/* Wlacz */
DEF_MSG_6(50, kondPom, cSharc, cPWM, urzadzenie, guiTp);
typedef msg_50_t msg_50r_t;
typedef msg_50_t msg_51_t;
typedef msg_51_t msg_51r_t;

/* Wylacz */
DEF_MSG_6(52, kondPom, zero1, zero2, urzadzenie, guiTp);
typedef msg_52_t msg_52r_t;
typedef msg_52_t msg_53_t;
typedef msg_53_t msg_53r_t;

/* Sprawdzenie polaczenia z MA */
#define FLAGS55_IS_TWO_OUTPUS     0x01
#define FLAGS55_AM_I_STEREO       0x02
#define FLAGS55_SMOK_ON           0x04
#define FLAGS55_DEFAULT_PIPE      0x08
#define FLAGS55_CONNECTION_ACTIVE 0x10
DEF_MSG_15(55, kondPom, pipeNum, flags55, uptime3, uptime2, uptime1, uptime0, cSharc, cPWM, signalType, outputType, volume, urzadzenie, guiTp);
typedef msg_55_t msg_55r_t;

#ifdef SMOK
/* Wylacz PWM*/
DEF_MSG_1(56);
typedef msg_56_t msg_56r_t;
typedef msg_56_t msg_57_t;
typedef msg_57_t msg_57r_t;
#endif

/* Komendy konfiguracyjne Sharc */
DEF_MSG_6(58, kondPom, Sharc_komenda, msg60, urzadzenie, guiTp);
typedef msg_58_t msg_58r_t;
typedef msg_58_t msg_59_t;
typedef msg_59_t msg_59r_t;

/* do PWM */
DEF_MSG_6(5A, kondPom, zero1, PWM_komenda, urzadzenie, guiTp);
typedef msg_5A_t msg_5Ar_t;
typedef msg_5A_t msg_5B_t;
typedef msg_5B_t msg_5Br_t;

/* Zwrotne info Sharc */
#if defined(MADO)
DEF_MSG_7(60, kondPom, urzadzenie, Sharc_msg2, Sharc_msg1, Sharc_Msg, guiTp);
DEF_MSG_6(60r, kondPom, urzadzenie, Sharc_msg2, Sharc_msg1, Sharc_Msg);
#elif defined(SMOK)
DEF_MSG_6(60, kondPom, urzadzenie, Sharc_msg2, Sharc_msg1, Sharc_Msg);
typedef msg_60_t msg_60r_t;
#endif

/* Zwrotne info PWM */
#if defined(MADO)
DEF_MSG_7(70, kondPom, urzadzenie, PWM_msg2, PWM_msg1, PWM_Msg, guiTp);
DEF_MSG_6(70r, kondPom, urzadzenie, PWM_msg2, PWM_msg1, PWM_Msg);
#elif defined(SMOK)
DEF_MSG_6(70, kondPom, urzadzenie, PWM_msg2, PWM_msg1, PWM_Msg);
typedef msg_70_t msg_70r_t;
#endif

/* Glosnosc - komenda */
DEF_MSG_6(80, kondPom, urzadzenie, volume, zero1, guiTp);
typedef msg_80_t msg_80r_t;
typedef msg_80_t msg_81_t;
typedef msg_81_t msg_81r_t;

#if defined(MADO)
/* Zapytanie o aktywne nadajniki */
/* Informacja o aktywnym nadajniku lub zestawie glsnikowym */
DEF_MSG_8(90, kondPomMADO, ZkondPomA1, ZkondPomA2, konfig_ZG, ZkondPomB1, ZkondPomB2, guiTp);
typedef msg_90_t msg_90r_t;
typedef msg_90_t msg_91_t;
typedef msg_91_t msg_91r_t;
#endif

/* Polecenie od MA zmiany przypisania odbiornikow (zmiana trybu pracy MA (CD/HD/UHD)) */
DEF_MSG_10(92, kondPom, signalType, outputType, device, unused1,  unused2,  unused3,  unused4, guiTp);
typedef msg_92_t msg_92r_t;
typedef msg_92_t msg_93_t;
typedef msg_93_t msg_93r_t;

#if defined(MADO)
/* Konfiguracja wejsc MA */
DEF_MSG_6(94, kondPom, konfigWejsc, msg, urzadzenie, guiTp);
typedef msg_94_t msg_94r_t;

/* Polecenie polaczenia do zestawu gooonnnnnnikowego */
DEF_MSG_10(96, kondPom, cSharc, cPWM, gStatus, VolumeL, VolumeR, CD_HD, MA_OUT, guiTp);
typedef msg_96_t msg_96r_t;

/* Polecenie odlaczenia od jednego zestawu glosnikowego */
DEF_MSG_6(98, kondPom, zero1, zero2, zero3, guiTp);
typedef msg_98_t msg_98r_t;

/* Komunikaty o statusie sieci (diagnostyczne) */
DEF_MSG_6(A0, kondPom, urzadzenie, radioMsg1, radioMsg2, guiTp);
typedef msg_A0_t msg_A0r_t;
#endif

/* Polecenie programowania modulow radiowych */
DEF_MSG_7(9C, kondPom, man_id3, man_id2, man_id1, flags, guiTp);
#define MSG9C_FLAGS_PIPE_MASK      0x01
#define MSG9C_FLAGS_LEFT_MASK      0x02
#define MSG9C_FLAGS_MASTER_MASK    0x10
typedef msg_9C_t msg_9Cr_t;
typedef msg_9C_t msg_9D_t;
typedef msg_9D_t msg_9Dr_t;

DEF_MSG_TABLE(A2, data, 31);
DEF_MSG_TABLE(A2r, data, 5);
typedef msg_A2r_t msg_A3_t;
typedef msg_A3_t msg_A3r_t;
enum
{
  FILE_NONE,
  FILE_STM,
  FILE_SHARC,
  FILE_BUFFER,
  FILE_INIT,
};
typedef struct
{
  uint8_t code;
  uint8_t subCode;
  uint16_t mask;
  union
  {
    struct
    {
      uint16_t chunkIdx;
      uint8_t data[24];
      uint16_t crc;
    } __attribute__((packed))cmd_buffer;
    struct
    {
      uint32_t size;
      uint32_t crc32;
    } __attribute__((packed))cmd_finish;
  } __attribute__((packed));
} __attribute__((packed))A2Data_t;


/* Komenda statusowa kolumna */
#if defined(MADO)
DEF_MSG_6(B0r, kondPom, urzadzenie, Smok_msgType, Smok_msg1, Smok_msg2);
DEF_MSG_7(B0, kondPom, urzadzenie, Smok_msgType, Smok_msg1, Smok_msg2, guiTp);

DEF_MSG_8(B1r, kondPom, urzadzenie, Smok_msgType, Smok_msgA1, Smok_msgA2, Smok_msgB1, Smok_msgB2);
DEF_MSG_9(B1, kondPom, urzadzenie, Smok_msgType, Smok_msgA1, Smok_msgA2, Smok_msgB1, Smok_msgB2, guiTp);

#elif defined(SMOK)
DEF_MSG_6(B0, kondPom, urzadzenie, Smok_msgType, Smok_msg1, Smok_msg2);
typedef msg_B0_t msg_B0r_t;

DEF_MSG_8(B1, kondPom, urzadzenie, Smok_msgType, Smok_msgA1, Smok_msgA2, Smok_msgB1, Smok_msgB2);
typedef msg_B1_t msg_B1r_t;
#endif

#if defined(SMOK)
/* Komenda monitoringu polaczenia urzadzen typu PER z punktu widzenia Smok-a */
DEF_MSG_1(CC);
typedef msg_CC_t msg_CCr_t;
typedef msg_CC_t msg_C1_t;
typedef msg_CC_t msg_C2_t;
typedef msg_C1_t msg_C1r_t;
typedef msg_C2_t msg_C2r_t;
#endif

typedef enum
{
  NET_JOINED,
  NET_DATAGRAM_CONNECTED,
  NET_DATAGRAM_LOST,
  NET_DROPPED,
} netStatus_e;
//1 - forward (CEN->PER)
//0 - backward (PER->CEN)
#define DIRECTION(arg_portNum) (arg_portNum == CEN_PORT)

int sendNetworkStatus(portNum_t portNum, netStatus_e status, uint8_t lastByte);


typedef enum
{
  NET_NC,
  NET_PARTIAL,
  NET_DATAGRAM,
} netState_e;
void connectionState(netState_e state);

void printfCommStats(void);
void sendKANow(portNum_t portNum);

portNum_t getSecondPort(portNum_t portNum);
void lockNonFlashMessages(uint8_t lock);
