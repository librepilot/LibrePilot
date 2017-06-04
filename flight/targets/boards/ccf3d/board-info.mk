BOARD_TYPE          := 0x04
BOARD_REVISION      := 0x03
BOOTLOADER_VERSION  := 0x04
HW_TYPE             := 0x01

CHIP                := STM32F303xC

OPENOCD_JTAG_CONFIG := foss-jtag.revb.cfg
OPENOCD_CONFIG      := stm32f3x.cfg

# Note: These must match the values in link_$(BOARD)_memory.ld
BL_BANK_BASE        := 0x08000000  # Start of bootloader flash
BL_BANK_SIZE        := 0x00004000  # Should include BD_INFO region
FW_BANK_BASE        := 0x0800C000  # Start of firmware flash
FW_BANK_SIZE        := 0x00034000  # Should include FW_DESC_SIZE (208kB)

FW_DESC_SIZE        := 0x00000064

OSCILLATOR_FREQ     :=   8000000
