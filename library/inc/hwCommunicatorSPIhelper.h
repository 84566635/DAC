#ifndef HW_COMMUNICATOR_SPI_HELPER_H_
#define HW_COMMUNICATOR_SPI_HELPER_H_

#include "cc85xx_common.h"

int SPIreceiveData(portNum_t portNum, char *buffer, int maxSize);
int SPIsendData(portNum_t portNum, char *buffer, int size);

typedef struct
{
  GPIO_TypeDef *gpio;
  uint16_t      pin;
} gpio_t;

typedef struct
{
  void *spi;
  gpio_t cs;
  gpio_t mosi;
  gpio_t miso;
  gpio_t reset;
} spi_t;

#define MAN_ID_UNIQUE_BIT  8 //Unique number to distinguish sets of speakers
#define MAN_ID_UNIQUE_MASK 0xFFFFFF00
#define MAN_ID_STEREO_BIT  0 //Single Slave or stereo mode in multislave
#define MAN_ID_STEREO_MASK 0x000000001
#define MAN_ID_LR_BIT      1 //Left or right radio module for stereo or first/second stream for multislave
#define MAN_ID_LR_MASK     0x000000002


#define PROD_ID_LR_BIT      0 //Left or right radio module for stereo or first/second stream for multislave
#define PROD_ID_LR_MASK     0x0000000FF

#define PROD_ID_PRIMARY_STREAM_BIT   16 //Primary stream 0/1
#define PROD_ID_PRIMARY_STREAM_MASK     0x000010000

#define PROD_ID_KONDPOM_BIT 8 //kondgnacja pomieszczenie
#define PROD_ID_KONDPOM_MASK 0x00000FF00

typedef struct
{
  //SPI port used for the radio
  spi_t spi;
  device_t device;
  uint32_t txFull;
  uint32_t txNoConn;
  uint32_t smplRate;
  uint32_t tsPeriod;
  uint16_t status;
  uint8_t  nwkStateChange;
} radioModule_t;

typedef struct
{
  roundRobin_t rr;
  radioModule_t *radioModule;
  //Number of the module used with the port
  int moduleNum;

  //Radio device data:
  uint8_t master;
  remoteDev_t connDev;

#ifdef SMOK
  //Primary and secondary radio device to connect with
  remoteDev_t remote[2];
  int currentRemote;
#endif

} spiHelper_t;

extern radioModule_t radioModule[];

void configureDMAforSPI(void *spi, void *txBuffer, void *rxBuffer, int size);
void spiHelperInit(void);
#define SPI_EV_TYPE_DATA_READY       0
#define SPI_EV_TYPE_LINK_ERROR       1
#define SPI_EV_TYPE_DLINK_ERROR      2
#define SPI_EV_TYPE_LINK_ESTABLISHED 3
int SPIevent(portNum_t portNum, uint8_t evType);
int radioReinit(void);

void slaveDisconnect(void);
int ehifStart(spi_t *spi);
void ehifStop(void);
#ifdef MADO
portNum_t getPortNum(int kondPom, int device);
#endif
#endif
