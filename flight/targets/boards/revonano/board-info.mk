BOARD_TYPE          := 0x09
BOARD_REVISION      := 0x05
BOOTLOADER_VERSION  := 0x06
HW_TYPE             := 0x00

MCU                 := cortex-m4
CHIP                := STM32F411CE
CHIPFAMILY			:= STM32F411xx
BOARD               := STM32F4xx_RN
MODEL               := HD
MODEL_SUFFIX        := 

OPENOCD_JTAG_CONFIG := stlink-v2.cfg
OPENOCD_CONFIG      := stm32f4xx.stlink.cfg

# Flash memory map for Revolution Nano:
# Sector	start		size	use
# 0		0x0800 0000	 16k	BL
# 1		0x0800 4000	 16k	BL
# 2		0x0800 8000	 16k	EE
# 3		0x0800 C000	 16k	EE
# 4		0x0801 0000	 64k	FW
# 5		0x0802 0000	128k	FW     --- Proto
# 6		0x0804 0000	128k	FW
# 7 	0x0806 0000	128k	FW


# Note: These must match the values in link_$(BOARD)_memory.ld
BL_BANK_BASE        := 0x08000000  # Start of bootloader flash
BL_BANK_SIZE        := 0x00008000  # Should include BD_INFO region

# 16KB for settings storage

EE_BANK_BASE        := 0x08008000  # EEPROM storage area
EE_BANK_SIZE        := 0x00008000  # Size of EEPROM storage area

# Leave the remaining 64KB sectors for other uses

FW_BANK_BASE        := 0x08010000  # Start of firmware flash
FW_BANK_SIZE        := 0x00070000  # Should include FW_DESC_SIZE

FW_DESC_SIZE        := 0x00000064

OSCILLATOR_FREQ     :=   8000000
SYSCLK_FREQ         :=  96000000
