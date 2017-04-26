#
# Rules to (help) build the F30x device support.
#

# Directory containing this makefile
PIOS_DEVLIB		:= $(dir $(lastword $(MAKEFILE_LIST)))

MCU := cortex-m4

USE_USB ?= YES

LDSRC = $(PIOS_DEVLIB)link_memory.lds $(PIOS_DEVLIB)link_sections.lds

CDEFS				+= -D$(CHIP) -DSTM32F3
CDEFS				+= -DHSE_VALUE=$(OSCILLATOR_FREQ)
CDEFS 				+= -DUSE_STDPERIPH_DRIVER
CDEFS				+= -DARM_MATH_CM4 -D__FPU_PRESENT=1
CDEFS			+= -DPIOS_TARGET_PROVIDES_FAST_HEAP

ARCHFLAGS		+= -mcpu=cortex-m4 -march=armv7e-m -mfpu=fpv4-sp-d16 -mfloat-abi=hard

# PIOS device library source and includes
PIOS_DEVLIB_SRC	= $(sort $(wildcard $(PIOS_DEVLIB)*.c))

ifeq ($(PIOS_OMITS_USB),YES)
    SRC += $(filter-out $(wildcard $(PIOS_DEVLIB)pios_usb*.c), $(PIOS_DEVLIB_SRC))
else
    SRC += $(PIOS_DEVLIB_SRC)
endif

EXTRAINCDIRS		+= $(PIOS_DEVLIB)inc

# CMSIS for the F3
include $(PIOSCOMMON)/libraries/CMSIS/library.mk
CMSIS_DEVICEDIR	:= $(PIOS_DEVLIB)libraries/CMSIS3/Device/ST/STM32F30x
EXTRAINCDIRS		+= $(CMSIS_DEVICEDIR)/Include

# ST Peripheral library
PERIPHLIB		=  $(PIOS_DEVLIB)libraries/STM32F30x_StdPeriph_Driver
ALL_SPL_SOURCES = $(sort $(wildcard $(PERIPHLIB)/src/*.c))
SPL_SOURCES = $(filter-out %stm32f30x_can.c, $(ALL_SPL_SOURCES))
SRC			+= $(SPL_SOURCES)
EXTRAINCDIRS		+= $(PERIPHLIB)/inc

ifneq ($(PIOS_OMITS_USB),YES)
    # ST USB Device library
    USBDEVLIB		=  $(PIOS_DEVLIB)/libraries/STM32_USB-FS-Device_Driver
    SRC			+= $(sort $(wildcard $(USBDEVLIB)/src/*.c))
    EXTRAINCDIRS		+= $(USBDEVLIB)/inc
endif

#
# FreeRTOS
#
# If the application has included the generic FreeRTOS support, then add in
# the device-specific pieces of the code.
#
ifneq ($(FREERTOS_DIR),)
    FREERTOS_PORTDIR	:= $(FREERTOS_DIR)
    SRC				+= $(sort $(wildcard $(FREERTOS_PORTDIR)/portable/GCC/ARM_CM4F/*.c))
    EXTRAINCDIRS	+= $(FREERTOS_PORTDIR)/portable/GCC/ARM_CM4F
    include $(PIOSCOMMON)/libraries/msheap/library.mk
endif
