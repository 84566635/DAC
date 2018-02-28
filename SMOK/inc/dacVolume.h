
#ifndef INC_DACVOLUME_H_
#define INC_DACVOLUME_H_

#include <stdint.h>

/*
 * TBD
 * @return 1 when configuration in applicable and module is correctly initialized, 0 otherwise
 */
uint8_t DACVOLUME_Init();

/*
 * TBD
 */
void DACVOLUME_SetVolumes(uint8_t volumeLeft, uint8_t volumeRight);


#endif /* INC_DACVOLUME_H_ */
