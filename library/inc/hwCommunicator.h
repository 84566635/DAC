int messageTx(portNum_t portNum, char *buffer, int bufferSize, uint32_t handle);
typedef enum
{
  RXTX_COMPLETE_TX = 0x01,
  RXTX_COMPLETE_RX = 0x02,
  RXTX_COMPLETE_BOTH = RXTX_COMPLETE_RX | RXTX_COMPLETE_TX,
} rxTxComplete_e;

typedef enum
{
  PORT_TYPE_USART = 0,
  PORT_TYPE_SPI   = 1,
  PORT_TYPE_NONE,
} portType_e;
int portDataTxRxComplete(portNum_t portNum, portType_e portType, rxTxComplete_e dir);

//Fill up the structure for each port with send/receive callbacks and associated module
typedef struct
{
  //A callback to receive one or more bytes from the port.
  int (*receiveData[2]) (portNum_t portNum, char *buffer, int maxSize);
  //A callback to transmit one or more bytes to the port.
  int (*sendData[2]) (portNum_t portNum, char *buffer, int size);
  //A handle to be used when the same functions are to be used for different devices
  void *SPIhelper;
  void *UARThelper;
  //port type
  portType_e portType;
  uint8_t kond_pom;
  uint8_t urzadzenie;
  uint8_t enabled;
  int    reqPipe;

#define DEV_UCONN 3 //USART
#define DEV_PCONN 2 //SPI Partial
#define DEV_CONN  1 //SPI Full
#define DEV_NCONN 0 //No connection
  uint8_t connState;
  uint8_t prevConnState;

  uint32_t keepAlive;//keep alive timetamp
  uint8_t ka;//Keepalive counter. No need to be large
  uint32_t tsConnected;
  uint32_t tsConnectedDatagram;
  uint32_t uptime;
  uint32_t tsLastSeen;
  uint32_t tsLastSeenSubPort[2];
#define IS_CON_TYPE_ACTIVE(arg_portNum, arg_type)  (portData[arg_portNum].tsLastSeenSubPort[arg_type] + 5000 > xTaskGetTickCount())

  uint32_t txFullDrop;
  uint32_t dscResetRx;
  uint32_t dscResetTx;
} portData_t;
extern portData_t portData[PORTS_NUM];

void HWCommInit(void);

#define PORT_DATA(arg_type, arg_SPIhelper, arg_UARThelper, arg_kondPom, arg_urzadzenie, arg_enabled) \
  {									\
    .portType = PORT_TYPE_##arg_type,					\
      .SPIhelper = arg_SPIhelper,					\
      .UARThelper = arg_UARThelper,					\
      .kond_pom = arg_kondPom,						\
      .connState = DEV_NCONN,						\
      .prevConnState = DEV_NCONN,					\
      .urzadzenie = arg_urzadzenie,					\
      .enabled = arg_enabled,						\
      .reqPipe = -1,							\
      .receiveData[PORT_TYPE_SPI] = SPIreceiveData,			\
      .receiveData[PORT_TYPE_USART] = USARTreceiveData,			\
      .sendData[PORT_TYPE_SPI] = SPIsendData,				\
      .sendData[PORT_TYPE_USART] = USARTsendData,			\
      }

int switchPortHelper(portNum_t p, int type);

#ifndef ROUND_ROBIN_SIZE
# define ROUND_ROBIN_SIZE 256
#endif
typedef struct
{
  int head;
  int tail;
  int num;
  char buffer[ROUND_ROBIN_SIZE];
} roundRobin_t;
int rrGet(roundRobin_t *rr);
int rrPut(roundRobin_t *rr, char ch);
#define TO_PRODUCT_ID(arg_kond_pom, arg_urzadzenie) ((uint32_t)(arg_kond_pom) | (uint32_t)(arg_urzadzenie))
int NONEreceiveData(portNum_t portNum, char *buffer, int maxSize);
int NONEsendData (portNum_t portNum, char *buffer, int size);
