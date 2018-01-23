#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/poll.h>
//#include <sys/types.h>
//#include <sys/socket.h>
//#include <termios.h>
//#include <sys/ioctl.h>
//#include <sys/poll.h>
//#include <sys/time.h>
//#include <sys/resource.h>
#include <string.h>
#include <stdlib.h>

static uint32_t crc32_tab[] =
{
  0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
  0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
  0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
  0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
  0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
  0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
  0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
  0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
  0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
  0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
  0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
  0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
  0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
  0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
  0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
  0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
  0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
  0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
  0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
  0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
  0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
  0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
  0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
  0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
  0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
  0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
  0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
  0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
  0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
  0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
  0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
  0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
  0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
  0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
  0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
  0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
  0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
  0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
  0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
  0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
  0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
  0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
  0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};
static const uint16_t crc16_tab[] =
{
  0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
  0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
  0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
  0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
  0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
  0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
  0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
  0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
  0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
  0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
  0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
  0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
  0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
  0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
  0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
  0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
  0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
  0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
  0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
  0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
  0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
  0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
  0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
  0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
  0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
  0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
  0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
  0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
  0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
  0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
  0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
  0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040,
};
#define CRC_FUNC(arg_len, arg_table, arg_invert)                        \
  uint##arg_len##_t crc##arg_len(uint##arg_len##_t crc, const void *buf, size_t size) \
  {                                                                     \
  const uint8_t *p = (uint8_t*)buf;                                   \
  crc = (arg_invert)?crc ^ ~0U:crc;                                   \
  while (size--) crc = arg_table[(crc ^ *p++) & 0xFF] ^ (crc >> 8);   \
  return (arg_invert)?crc ^ ~0U:crc;                                  \
  }


static CRC_FUNC(32, crc32_tab, 1)
static CRC_FUNC(16, crc16_tab, 0)


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
  uint8_t mask;
  uint8_t acked;
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
static A2Data_t allBuffs[(sizeof(A2Data_t)==32)?(2048*1024/24):-1] = {0};



static const int pktSize[256] =
{
  [0 ... 255] = 0,
  [0xb0] = 7,
  [0xA0] = 6,
  [0xA2] = 6,
  [0xA3] = 6,
  [0x92] = 10,
  [0x93] = 10,
};


static int getRespond(int fd, int timeoutms)
{
  return 1;
  uint8_t buf[64];
  int i;
  for(i = 0; i< timeoutms; i++)
    {
      int readSize =0;
      int idx=0;
      int oneSize = 0;
      do
        {
          oneSize = read(fd, &buf[idx], sizeof(A2Data_t)-idx);
          idx+=oneSize;
          readSize += oneSize;
          usleep(1000);
        }
      while(idx < 32 && oneSize);
      A2Data_t *ptr = (void*)buf;
      while(readSize>0)
        {
          switch(ptr->code)
            {
            case 0xA2:
              return ptr->cmd_buffer.chunkIdx+1;
            case 0xA3:
              return -ptr->cmd_buffer.chunkIdx-1;
            default:
              if(pktSize[ptr->code] > 0)
                {
                  readSize -= pktSize[ptr->code];
                  ptr+= pktSize[ptr->code];
                }
              else
                {

                  printf("%5d: got %d bytes ", i, readSize);
                  int j;
                  for(j = 0; j< readSize; j++)
                    printf("%02x ", ((uint8_t*)ptr)[j]);
                  printf("\n");
                  readSize = 0;
                }
              break;
            }
        }
    }
  return -0xF0000;//Timeout
}
static void hexDump(void* ptr, int size, int rowSize)
{
  int cnt = 0;
  int row = 0;
  for(cnt=0; cnt<size; cnt++)
    {
      if((cnt%rowSize) == 0)
        printf("\n%10p (%02X)[%02X]: ",ptr, cnt, row++);
      printf("%02X ",((uint8_t*)ptr)[cnt]);
    }
}








static int msgSize[256] =
{
  [0 ... 255] = 0,
  [0x28]= 6,
  [0x20]= 6,
  [0x48]= 6,
  [0x49]= 6,
  [0x50]= 6,
  [0x51]= 6,
  [0x52]= 6,
  [0x53]= 6,
  [0x58]= 6,
  [0x59]= 6,
  [0x5A]= 6,
  [0x5B]= 6,
  [0x60]= 7,
  [0x70]= 7,
  [0x80]= 6,
  [0x81]= 6,
  [0x90]= 8,
  [0x92]= 10,
  [0x94]= 6,
  [0x95]= 6,
  [0x96]= 10,
  [0x97]= 10,
  [0x98]= 6,
  [0x99]= 6,
  [0x9C]= 7,
  [0x9D]= 7,
  [0xA0]= 6,
  [0xB0]= 7,
  [0xB1]= 9,
  [0xF0]= 0,
  [0xF1]= 6,
  [0xF2]= 3,
  [0xF3]= 3,
  [0xA2] = sizeof(A2Data_t),
};

static pthread_mutex_t mutex =  PTHREAD_MUTEX_INITIALIZER;

static int fillSize = 0;

static int ackInit = 0;
static int ackFinish = 0;


static void* rx_thread(void *ptr)
{
  int ttyFD = (int)ptr;
  pthread_t this_thread = pthread_self();
  struct sched_param sched_params;
  int a;
  pthread_getschedparam(this_thread, &a, &sched_params);
  sched_params.sched_priority = sched_get_priority_max(SCHED_FIFO);
  if (pthread_setschedparam(this_thread, SCHED_FIFO, &sched_params) != 0)
    printf("Unsuccessful in setting thread priority\n");

  struct pollfd my_fds;
  my_fds.fd = ttyFD;
  my_fds.events = POLLIN;
  my_fds.revents = 0;

  struct timeval act_time;
  gettimeofday(&act_time, NULL);
  act_time.tv_sec += 1000;


  uint8_t buf[64] = "x";
  while(1)
    {
      int oneSize = 0;
      my_fds.revents = 0;
      while(poll(&my_fds, 1, 1000) == 0)
        {
          //Timeout??
          pthread_mutex_lock(&mutex);
          fillSize=0;
          pthread_mutex_unlock(&mutex);
        }

      struct timeval now_time;
      gettimeofday(&now_time, NULL);
      if(act_time.tv_sec < now_time.tv_sec)
        {
          //Timeout??
          pthread_mutex_lock(&mutex);
          fillSize=0;
          pthread_mutex_unlock(&mutex);
          act_time.tv_sec = now_time.tv_sec;
        }


      int idx =0;
      int maxSize = 32;

      while(maxSize > idx && (oneSize = read(ttyFD, &buf[idx], maxSize-idx)) > 0)
        {
          if(idx == 0)
            maxSize = msgSize[buf[0]];
          idx += oneSize;
          my_fds.revents = 0;
          poll(&my_fds, 1, 1);
        }
      if(buf[0] == 0xA2)
        {
          gettimeofday(&act_time, NULL);

          A2Data_t *rxA20 = (void*)buf;
          switch(rxA20->subCode)
            {
            case FILE_BUFFER:
            {
              int idx = rxA20->cmd_buffer.chunkIdx;
              allBuffs[idx].acked = 1;

              pthread_mutex_lock(&mutex);
              if(fillSize > 0)
                fillSize--;
              pthread_mutex_unlock(&mutex);
            }
            break;
            case FILE_INIT:
              ackInit = 1;
              break;
            case FILE_SHARC:
            case FILE_STM:
              ackFinish = 1;
              break;
            }
        }
    }
}


int main(int argc, char **argv)
{
  if(argc < 5)
    {
      return 1;
    }
  int type = FILE_NONE;

  if(strcasecmp(argv[1], "STM") == 0)
    type = FILE_STM;
  else if(strcasecmp(argv[1], "SHARC") == 0)
    type = FILE_SHARC;
  else
    return 1;
  uint32_t perMask = strtol(argv[2], NULL, 2);
  char *ttyName = argv[4];
  char *fileName = argv[3];
  int ttyFD = open(ttyName, O_RDWR | O_NOCTTY | O_NONBLOCK);
  if(ttyFD < 0)
    return 2;

  pthread_t thread1;
  pthread_create (&thread1, NULL, rx_thread, (void*)ttyFD);

  int fileFD = open(fileName, O_RDWR);
  if(fileFD < 0)
    {
      close(ttyFD);
      return 3;
    }
  //Get buffer fir the file. The image must not exceed 1MB as this is max storage on STM
#define MAX_FILESIZE (1024*1024)
  uint8_t *fileBuffer = calloc(1, MAX_FILESIZE);
  if(!fileBuffer)
    {
      close(ttyFD);
      close(fileFD);
      return 4;
    }
  printf("Flash file %s sending to %s  (%s mask %s) \n", fileName, ttyName, argv[1], argv[2]);
  int fileSize = read(fileFD, fileBuffer, MAX_FILESIZE);
  close(fileFD);

  uint8_t sendBuf[64];
  A2Data_t *buf = (A2Data_t*)sendBuf;
  int retry;
  //Send INIT
  buf->code = 0xA2;
  buf->subCode = FILE_INIT;
  buf->mask = perMask;
  write(ttyFD, sendBuf, sizeof(A2Data_t));
  printf("Init start\n");
  retry = 100;
  while(retry-- && ackInit == 0) usleep(100000);
  if(ackInit == 0)
    {
      printf("Init failed\n");
      return 1;
    }
  printf("Init end\n");

  //Send file to the MADO
  int i=0;
  int nacks = 0;
  int acks = 0;
  int timeouts = 0;
  int sent = 0;
#define MAX_FILL 4

  uint32_t crc = crc32(0, fileBuffer, fileSize);
  int fragNum = (fileSize + sizeof(buf->cmd_buffer.data) -1)/sizeof(buf->cmd_buffer.data);


  //Fillup table
  for(i=0; i < fragNum; i++)
    {
      allBuffs[i].code = 0xA2;
      allBuffs[i].subCode = FILE_BUFFER;
      allBuffs[i].mask = perMask;
      allBuffs[i].acked = 0;
      allBuffs[i].cmd_buffer.chunkIdx = i;
      allBuffs[i].cmd_buffer.crc = 0;
      memcpy(allBuffs[i].cmd_buffer.data, &fileBuffer[(i)*sizeof(allBuffs[i].cmd_buffer.data)], sizeof(allBuffs[i].cmd_buffer.data));
      allBuffs[i].cmd_buffer.crc = crc16(0, &allBuffs[i], sizeof(A2Data_t));
    }
  free(fileBuffer);


  retry = 3;
  int sendOK = 0;
  int perc;
  while(retry-- > 0 && sendOK == 0)
    {
      perc = 0;
      //Send all packets
      for(i=0; i < fragNum; i++)
        {
          while(fillSize >= MAX_FILL)
            usleep(1000);
          int newPerc = (100*i)/fragNum;
          if(newPerc > perc)
            {
              perc = newPerc;
              printf("\r Try %d, sent %2d%% ", 3-retry,  perc);
              fflush(stdout);
            }
          if(allBuffs[i].acked == 0)
            {
              write(ttyFD, &allBuffs[i], sizeof(A2Data_t));
              pthread_mutex_lock(&mutex);
              fillSize++;
              pthread_mutex_unlock(&mutex);
            }
        }

      //verify acknowledgement
      sendOK = 1;
      for(i=0; i < fragNum; i++)
        {
          while(fillSize > 0)
            usleep(1000);

          if(allBuffs[i].acked == 0)
            {
              sendOK = 0;
              break;//Retry sending
            }
        }
    }

  printf("\nStart programming\n");
  //Send finish
  buf->code = 0xA2;
  buf->subCode = type;
  buf->mask = perMask;
  buf->cmd_finish.crc32 = crc;
  buf->cmd_finish.size = fileSize;
  write(ttyFD, sendBuf, sizeof(A2Data_t));
  retry = 600;
  while(retry-- && ackFinish == 0) usleep(100000);
  if(ackFinish == 0)
    {
      printf("\nop failed!!!!!\n");
      return 1;
    }
  printf("\nFlashing ok\n");


  close(ttyFD);
  return 0;
}

