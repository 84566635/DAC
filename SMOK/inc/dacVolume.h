
#ifndef INC_DACVOLUME_H_
#define INC_DACVOLUME_H_

#include <stdint.h>

/*
 * Initialize DACVOLUME module, starting new FreeRTOS thread "dacVolume".
 * Module starts only if system configuration variable 'centralDeviceMode' is set to DEVMODE_DAC.
 * This module uses software-gpio SPI to communicate with external device.
 * It sends left/right volume every DACVOLUME_PERIOD_MS period.
 * Module timing relay on SystemCoreClock external variable.
 * @return 1 when configuration in applicable and module is correctly initialized, -1 otherwise
 */
int DACVOLUME_Init();

/*
 * Set left/right volume to be sent by module every DACVOLUME_PERIOD_MS period.
 * Function enter critical section to exchange data in atomic way.
 * Thread safe.
 * @param volumeLeft parameter value will be copied in critical section to module memory.
 * @param volumeRight as above
 */
void DACVOLUME_SetVolumes(uint8_t volumeLeft, uint8_t volumeRight);


#endif /* INC_DACVOLUME_H_ */
