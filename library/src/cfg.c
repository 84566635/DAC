#include <project.h>
#include <common.h>
#include <cfg.h>
#include STMINCP()
#include STMINCS(_flash)
#include <crc.h>

config_t cfg __attribute__((section(".cfg")));
static int cfgSizeTest[(sizeof(cfgnvs) == 4*128)?0:-1];
void cfgInit(void)
{
  (void)cfgSizeTest;
  uint8_t *src = (uint8_t*)&cfgnvs;
  uint8_t *dst = (uint8_t*)&cfg;
  int idx;
  for(idx = 0; idx < sizeof(cfg); idx++)
    dst[idx] = src[idx];
}
void cfgFlush(void)
{
  FLASH_ClearFlag(FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR );
  FLASH_Unlock();
  FLASH_EraseSector(FLASH_Sector_1, VoltageRange_3);
  uint32_t *src = (uint32_t*)&cfg;
  uint32_t addr = (uint32_t)&cfgnvs;
  uint32_t eaddr = (uint32_t)&cfgnvs + sizeof(cfgnvs);
  for(; addr < eaddr; addr+=4)
    FLASH_ProgramWord(addr, *(src++));
}

uint8_t __attribute__((section(".storage")))flashStorage[0];
extern uint8_t _nvs[0];
void updateFW(void)
{
  cfgInit();
  if(cfg.fSize > 0)
    {
      uint32_t crc = crc32(0, flashStorage, cfg.fSize);
      if((int)crc != cfg.fCRC)
        return;

      //Update FW
      FLASH_ClearFlag(FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR );
      FLASH_Unlock();
      FLASH_EraseSector(FLASH_Sector_1, VoltageRange_3);
      FLASH_EraseSector(FLASH_Sector_2, VoltageRange_3);
      FLASH_EraseSector(FLASH_Sector_3, VoltageRange_3);
      FLASH_EraseSector(FLASH_Sector_4, VoltageRange_3);
      FLASH_EraseSector(FLASH_Sector_5, VoltageRange_3);
      FLASH_EraseSector(FLASH_Sector_6, VoltageRange_3);
      FLASH_EraseSector(FLASH_Sector_7, VoltageRange_3);
      uint32_t *src = (uint32_t*)(&flashStorage[16*1024]);//Omit bootloader
      uint32_t addr = (uint32_t)(&_nvs[0]);
      int num =  (cfg.fSize+3-16*1024)/4;
      while(num--)
        {
          FLASH_ProgramWord(addr, *src);
          src++;
          addr+=4;
        }
    }
}
