/** \addtogroup module_ehif_hal_mcu HAL: Microcontroller Specific Definitions and Routines
 *
 * \brief Defines microcontroller specific constants, macros and functions
 *
 * \section section_ehif_defs_overview Overview
 * The following items are defined here:
 * - SPI status word events
 * - SPI command type IDs
 * - Data structure types for CMD_REQ parameters and READ/READBC/WRITE data
 * - Buffer union types of all CMD_REQ parameter structures and all READ/READBC/WRITE data structures
 *
 * @{
 */
#ifndef CC85XX_EHIF_HAL_MCU_H_
#define CC85XX_EHIF_HAL_MCU_H_

#include <cc85xx_ehif_hal_board.h>
#include <FreeRTOS.h>
#include <task.h>

//-------------------------------------------------------------------------------------------------------
/// \name Delay Insertion
//@{

/// Inserts a delay lasting for at least the specified number of milliseconds
#define EHIF_DELAY_MS(x)     st(vTaskDelay(x);)
/// Inserts a delay lasting for at least the specified number of microseconds
void EHIF_DELAY_US(int x);

//@}
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
/// \name Critical Section Handling
//@{
void spiIRQEnable(int enable);

/// Starts a critical code section by disabling interrupts globally
#define EHIF_ENTER_CRITICAL_SECTION(){taskENTER_CRITICAL();spiIRQEnable(0);}


/// Ends a critical code section by re-enabling interrupts globally
#define EHIF_LEAVE_CRITICAL_SECTION(){spiIRQEnable(1);taskEXIT_CRITICAL();}

//@}
//-------------------------------------------------------------------------------------------------------


#endif
//@}
