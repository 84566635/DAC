

MEMORY
{
  FLASH_BLD (rx) : ORIGIN = 0x08000000,      LENGTH = 16K
  FLASH_NVS (rx) : ORIGIN = 0x08000000+16K,  LENGTH = 16K
  FLASH0 (rx) :    ORIGIN = 0x08000000+32K,  LENGTH = 512K-32K
  FLASH1 (rx) :    ORIGIN = 0x08000000+512K, LENGTH = 512K
  SRAM0  (rwx): ORIGIN = 0x20000000, LENGTH = 112K
  SRAM1  (rwx): ORIGIN = 0x10000000, LENGTH = 64K
  SRAM2  (rwx): ORIGIN = 0x2001C000, LENGTH = 16K
}

ENTRY(Reset_Handler)

SECTIONS
{
	.bootloader :
	{
		KEEP(*(.resetisr))
		*(.bootloader*)
		*/cfg.o(.text*)
		*/cfg.o(.rodata*)
		*/crc.o(.text*)
		*/crc.o(.rodata*)
		*/stm32f4xx_flash.o(.text*)
		*/stm32f4xx_flash.o(.rodata*)
	} > FLASH_BLD	
	
	.nvs :
	{
	        _nvs = .;
		KEEP(*(.nvs))
	} > FLASH_NVS	
	
	.text :
	{
		_new_image = .;
		KEEP(*(.isr))
		KEEP(*(.init))
		KEEP(*(.fini))

		*(.text*)
		*(.rodata*)

		KEEP(*(.eh_frame*))
	} > FLASH0
	.ARM.extab :
	{
		*(.ARM.extab* .gnu.linkonce.armextab.*)
	} > FLASH0

	.ARM.exidx :
	{
		*(.ARM.exidx* .gnu.linkonce.armexidx.*)
	} > FLASH0
	__etext = .;

	.stack (NOLOAD):
	{
		*(.stack*)
		*(.cfg*)
	} > SRAM1
	.noload (NOLOAD):
	{
		*(.heap*)
		*(.noload*)
	} > SRAM0
	.data : AT (__etext)
	{
		__sdata = .;
		*(.data*)

		. = ALIGN(4);
		*(.bss*)
		*(COMMON)
		__edata = .;

	} > SRAM0
	.storage :
	{
           *(.storage*)
	} > FLASH1
	
	
   .sram2_data (NOLOAD):
   {
     . = ALIGN(4);
     *(. sram2_data* )
     . = ALIGN(4);
   } >SRAM2
}
