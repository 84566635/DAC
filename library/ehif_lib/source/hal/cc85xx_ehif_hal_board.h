/** \addtogroup module_ehif_hal_board HAL: Board Specific Definitions and Routines
 * \ingroup module_ehif_mcu
 *
 * \brief Defines board specific constants, macros and functions
 *
 * \section section_ehif_hal_board_overview Overview
 * The following items are defined here:
 * - MCU clock speed
 * - Time constants that depend on MCU clock speed and SPI interface configuration
 * - Fundamental SPI operations
 * - Fundamental pin operations
 * - EHIF event interrupt handling
 *
 * @{
 */
#ifndef CC85XX_EHIF_HAL_BOARD_H_
#define CC85XX_EHIF_HAL_BOARD_H_

//-------------------------------------------------------------------------------------------------------
/// \name Clock Speed and Delay Definitions
//@{

/// Delay in us between SYS_RESET or BOOT_RESET SPI byte transfers and CSn high afterwards (0 = none)
#define EHIF_DELAY_SPI_RESET_TO_CSN_HIGH    2

//@}
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
/// \name SPI Interface Macros
//@{

/// Activates CSn, starting an SPI operation
void EHIF_SPI_BEGIN(void);

/// Non-zero when EHIF is ready, zero when EHIF is not ready
char EHIF_SPI_IS_CMDREQ_READY(void);

/// Transmits a single byte
void EHIF_SPI_TX(char x);

/// Waits for completion of \ref EHIF_SPI_TX() (no timeout required!)
void EHIF_SPI_WAIT_TXRX(void);

/// The received byte after completing the last \ref EHIF_SPI_TX()
char EHIF_SPI_RX(void);

/// Deactivates CSn, ending an SPI operation
  void EHIF_SPI_END(void);

/// Forces the MOSI pin to the specified level
void EHIF_SPI_FORCE_MOSI(char x);
        			
/// Ends forcing of the MOSI pin started by \ref EHIF_SPI_FORCE_MOSI()
void EHIF_SPI_RELEASE_MOSI(void);
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
/// \name Reset Interface Macros
//@{

/// Activates RESETn, starting pin reset
void EHIF_PIN_RESET_BEGIN(void);

/// Deactivates RESETn, ending pin reset
void EHIF_PIN_RESET_END(void);

//@}
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
/// \name Event Interrupt
//@{

/// Non-zero when the EHIF interrupt is active, zero when the EHIF interrupt is active
char EHIF_INTERRUPT_IS_ACTIVE(void);

//@}
//-------------------------------------------------------------------------------------------------------


void ehifIoInit(void);


#endif
//@}
