BOARD_TYPE          := 0x10
BOARD_REVISION      := 0x03
BOOTLOADER_VERSION  := 0x04
HW_TYPE             := 0x01

CHIP                := STM32F303xE

OPENOCD_JTAG_CONFIG := foss-jtag.revb.cfg
OPENOCD_CONFIG      := stm32f3x.cfg

BL_BANK_BASE        := 0x08000000  # Start of bootloader flash
BL_BANK_SIZE        := 0x00004000  # Should include BD_INFO region

# Some KB for settings storage (32kb)
EE_BANK_BASE        := 0x08004000  # EEPROM storage area
EE_BANK_SIZE        := 0x00008000  # EEPROM storage size

FW_BANK_BASE        := 0x0800C000  # Start of firmware flash
FW_BANK_SIZE        := 0x00034000  # Should include FW_DESC_SIZE (208kB)

FW_DESC_SIZE        := 0x00000064

OSCILLATOR_FREQ     :=   8000000
