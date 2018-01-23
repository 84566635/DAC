#ifndef _M25P20_H_
#define _M25P20_H_ 1
typedef struct
{
#define M25P20_CMD_WRITE_ENABLE  0x06
#define M25P20_CMD_WRITE_DISABLE 0x04
#define M25P20_CMD_READ_ID       0x9F
#define M25P20_CMD_READ_STATUS   0x05
#define M25P20_CMD_WRITE_STATUS  0x01
#define M25P20_CMD_READ_DATA     0x03
#define M25P20_CMD_READ_DATA_HS  0x0B
#define M25P20_CMD_PAGE_PROGRAM  0x02
#define M25P20_CMD_SECTOR_ERASE  0xD8
#define M25P20_CMD_BULK_ERASE    0xC7

#define M25P20_XFER_SIZE(arg_struct, arg_field) (1+sizeof(arg_struct->arg_field))
#define M25P20_XFER_ADDRESS(arg_field, arg_address) \
  (arg_field)[2] = (uint8_t)((arg_address)>>0);	    \
  (arg_field)[1] = (uint8_t)((arg_address)>>8);	    \
  (arg_field)[0] = (uint8_t)((arg_address)>>16);

  uint8_t cmd;
  union
  {
    struct
    {
      uint8_t manufacturerId;
      uint8_t memoryType;
      uint8_t memoryCapacity;
      uint8_t cfdLength;
      uint8_t cfd[16];
    } __attribute__((packed)) rId;
    struct
    {
      uint8_t data;
#define M25P20_STATUS_WIP  0
#define M25P20_STATUS_WEL  1
#define M25P20_STATUS_BP   2
#define M25P20_STATUS_SRWD 7
    } __attribute__((packed)) rwStatus;
    struct
    {
      uint8_t address[3];
      uint8_t data[256];
    } __attribute__((packed)) rData;
    struct
    {
      uint8_t address[3];
      uint8_t dummy;
      uint8_t data[];
    } __attribute__((packed)) rDataHS;
    struct
    {
      uint8_t address[3];
      uint8_t data[256];
    } __attribute__((packed)) programPage;
    struct
    {
      uint8_t address[3];
    } __attribute__((packed)) sectorErase;
  } __attribute__((packed));
} __attribute__((packed)) m25p20_cmd_t;

int m25p20Program(const uint8_t *src, int size);

#endif
