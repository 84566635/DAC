#ifndef _CFG_H_
#define _CFG_H_

#define assert_param(cond) if(!(cond)){volatile int stop=1; while(stop);};
#define configASSERT assert_param

#define VERSION "MA_undefined"

#define PRINT_CFG(arg_name) dprintf(LL_WARNING, "cfg.%s=0x%02x \n", #arg_name, cfg.arg_name)
#define CFG_IDX(arg_cfg) (offsetof(config_t, arg_cfg)/(sizeof(int)))
#define CFG_PATCH(arg_idx) (0xABCDEF00+(arg_idx))
#define CFG_EVAL(arg_name) .arg_name = CFG_PATCH(CFG_IDX(arg_name))
#include <project.h>
extern config_t cfg;
extern const config_t cfgnvs;
void cfgInit(void);
void cfgFlush(void);
void updateFW(void);
extern unsigned char flashStorage[0];

#endif
