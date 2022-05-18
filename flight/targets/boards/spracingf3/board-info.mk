BOARD_TYPE          := 0x10
BOARD_REVISION      := 0x01
BOOTLOADER_VERSION  := 0x00
HW_TYPE             := 0x01

MCU                 := cortex-m4
PIOS_DEVLIB         := $(PIOS)/stm32f30x
CHIP                := STM32F303xC
BOARD               := STM32F303CCT_SRF3_Rev1
MODEL               := HD

OPENOCD_JTAG_CONFIG := jlink.cfg
OPENOCD_CONFIG      := stm32f3x.cfg

# Note: These must match the values in link_$(BOARD)_memory.ld
BL_BANK_BASE        := 0x08000000  # Start of bootloader flash
BL_BANK_SIZE        := 0x00004000  # Should include BD_INFO region

FW_BANK_BASE        := 0x08004000  # Start of firmware flash
FW_BANK_SIZE        := 0x0003C000  # Should include FW_DESC_SIZE

FW_DESC_SIZE        := 0x00000064

OSCILLATOR_FREQ     :=   8000000
