BOARD_TYPE          := 0x09
BOARD_REVISION      := 0x10
BOOTLOADER_VERSION  := 0x06
HW_TYPE             := 0x00

MCU                 := cortex-m4
CHIP                := STM32F427VI
CHIPFAMILY          := STM32F427xx
BOARD               := STM32F4xx_RQ
MODEL               := HD
MODEL_SUFFIX        := 

OPENOCD_JTAG_CONFIG := board/stm32f429discovery.cfg
OPENOCD_CONFIG      := stm32f4xx.stlink.cfg

# Flash memory map for Revolution RaspiCopterHat:
#BANK1
# Sector	start		size	use
# 0		0x0800 0000	 16k	BL
# 1		0x0800 4000	 16k	BL
# 2		0x0800 8000	 16k	EE
# 3		0x0800 C000	 16k	EE
# 4		0x0801 0000	 64k	RSVD
# 5		0x0802 0000	128k	FW
# 6		0x0804 0000	128k	FW
# 7		0x0806 0000	128k	FW
# 8 		0x0808 0000	128k	FW
# 9		0x080A 0000	128k	FW
# 10		0x080C 0000	128k	FW
# 11		0x080E 0000	128k   	FW
#BANK2
# Sector	start		size	use
# 12		0x0810 0000	 16k	Unused
# 13		0x0810 4000	 16k	Unused
# 14		0x0810 8000	 16k	Unused
# 15		0x0810 C000	 16k	Unused
# 16		0x0811 0000	 64k	Unused
# 17		0x0812 0000	128k	Unused
# 18		0x0814 0000	128k	Unused
# 19		0x0816 0000	128k	Unused
# 20		0x0818 0000	128k	Unused
# 21		0x081A 0000	128k	Unused
# 22		0x081C 0000	128k	Unused
# 23		0x081E 0000	128k   	Unused


# Note: These must match the values in link_$(BOARD)_memory.ld
BL_BANK_BASE        := 0x08000000  # Start of bootloader flash
BL_BANK_SIZE        := 0x00008000  # Should include BD_INFO region

# 16KB for settings storage

EE_BANK_BASE        := 0x08008000  # EEPROM storage area
EE_BANK_SIZE        := 0x00008000  # Size of EEPROM storage area

# Leave the remaining 64KB sectors for other uses

FW_BANK_BASE        := 0x08020000  # Start of firmware flash
FW_BANK_SIZE        := 0x001e0000  # Should include FW_DESC_SIZE

FW_DESC_SIZE        := 0x00000064

OSCILLATOR_FREQ     :=  12000000
SYSCLK_FREQ         := 168000000
