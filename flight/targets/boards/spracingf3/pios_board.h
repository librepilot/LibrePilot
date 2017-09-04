/**
 ******************************************************************************
 *
 * @file       pios_board.h
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 *
 * @brief      Defines board hardware for the SPRacing F3 board.
 * @see        The GNU Public License (GPL) Version 3
 *
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef PIOS_BOARD_H
#define PIOS_BOARD_H
// ------------------------
// Timers and Channels Used
// ------------------------
/*
   Timer | Channel 1 | Channel 2 | Channel 3 | Channel 4
   ------+-----------+-----------+-----------+----------
   TIM1  |  Servo 4  |           |           |
   TIM2  |  RC In 5  |  RC In 6  |  Servo 6  |
   TIM3  |  Servo 5  |  RC In 2  |  RC In 3  |  RC In 4
   TIM4  |  RC In 1  |  Servo 3  |  Servo 2  |  Servo 1
   ------+-----------+-----------+-----------+----------
 */

// ------------------------
// DMA Channels Used
// ------------------------
/* Channel 1  -                                 */
/* Channel 2  -                                 */
/* Channel 3  -                                 */
/* Channel 4  -                                 */
/* Channel 5  -                                 */
/* Channel 6  -                                 */
/* Channel 7  -                                 */
/* Channel 8  -                                 */
/* Channel 9  -                                 */
/* Channel 10 -                                 */
/* Channel 11 -                                 */
/* Channel 12 -                                 */

// ------------------------
// BOOTLOADER_SETTINGS
// ------------------------
#define BOARD_READABLE                    true
#define BOARD_WRITABLE                    true
#define MAX_DEL_RETRYS                    3

// ------------------------
// WATCHDOG_SETTINGS
// ------------------------
#define PIOS_WATCHDOG_TIMEOUT             250
#define PIOS_WDG_REGISTER                 RTC_BKP_DR4
#define PIOS_WDG_ACTUATOR                 0x0001
#define PIOS_WDG_STABILIZATION            0x0002
#define PIOS_WDG_ATTITUDE                 0x0004
#define PIOS_WDG_MANUAL                   0x0008
#define PIOS_WDG_AUTOTUNE                 0x0010
#define PIOS_WDG_SENSORS                  0x0020

// ------------------------
// TELEMETRY
// ------------------------
#define TELEM_QUEUE_SIZE                  10

// ------------------------
// PIOS_LED
// ------------------------
#define PIOS_LED_HEARTBEAT                0
#define PIOS_BUZZER_ALARM                 1

// -------------------------
// System Settings
// -------------------------
#define PIOS_MASTER_CLOCK                 72000000

// -------------------------
// Interrupt Priorities
// -------------------------
#define PIOS_IRQ_PRIO_LOW                 12              // lower than RTOS
#define PIOS_IRQ_PRIO_MID                 8               // higher than RTOS
#define PIOS_IRQ_PRIO_HIGH                5               // for SPI, ADC, I2C etc...
#define PIOS_IRQ_PRIO_HIGHEST             4               // for USART etc...
// ------------------------
// PIOS_I2C
// See also pios_board.c
// ------------------------
#define PIOS_I2C_MAX_DEVS                 1
extern uint32_t pios_i2c_id;
#define PIOS_I2C_MAIN_ADAPTER             (pios_i2c_id)
#define PIOS_I2C_ESC_ADAPTER              (pios_i2c_id)
#define PIOS_I2C_MS56XX_INTERNAL_ADAPTER  (pios_i2c_id)
#define PIOS_I2C_HMC5X83_INTERNAL_ADAPTER (pios_i2c_id)
#define PIOS_I2C_MPU6000_ADAPTER          (pios_i2c_id)
#define PIOS_I2C_EXTERNAL_ADAPTER         (pios_i2c_id)

// ------------------------
// PIOS_BMP085
// ------------------------
#define PIOS_BMP085_OVERSAMPLING          3

// -------------------------
// SPI
//
// See also pios_board.c
// -------------------------
#define PIOS_SPI_MAX_DEVS   2

// -------------------------
// PIOS_USART
// -------------------------
#define PIOS_USART_MAX_DEVS 2

// -------------------------
// PIOS_COM
//
// See also pios_board.c
// -------------------------
#define PIOS_COM_MAX_DEVS  3

#ifdef PIOS_INCLUDE_WS2811
extern uint32_t pios_ws2811_id;
#define PIOS_WS2811_DEVICE (pios_ws2811_id)
#endif

// -------------------------
// ADC
// PIOS_ADC_PinGet(0) = Current sensor
// PIOS_ADC_PinGet(1) = RSSI sensor
// PIOS_ADC_PinGet(2) = Battery voltage
// -------------------------
#define PIOS_DMA_PIN_CONFIG                            \
    {                                                  \
        { GPIOA, GPIO_Pin_5, ADC_Channel_2, false }, /* ADC_1 / Current   */  \
        { GPIOB, GPIO_Pin_2, ADC_Channel_12, false }, /* ADC_2 / RSSI   */  \
        { GPIOA, GPIO_Pin_4, ADC_Channel_1, false }, /* Battery voltage */  \
    }

/* we have to do all this to satisfy the PIOS_ADC_MAX_SAMPLES define in pios_adc.h */
/* which is annoying because this then determines the rate at which we generate buffer turnover events */
/* the objective here is to get enough buffer space to support 100Hz averaging rate */
#define PIOS_ADC_NUM_CHANNELS     3
#define PIOS_ADC_MAX_OVERSAMPLING 32
#define PIOS_ADC_USE_ADC2         0

// ------------------------
// PIOS_RCVR
// See also pios_board.c
// ------------------------
#define PIOS_RCVR_MAX_DEVS           3
#define PIOS_RCVR_MAX_CHANNELS       12
#define PIOS_GCSRCVR_TIMEOUT_MS      100

// -------------------------
// Receiver PPM input
// -------------------------
#define PIOS_PPM_MAX_DEVS            1
#define PIOS_PPM_NUM_INPUTS          16

// -------------------------
// Receiver PWM input
// -------------------------
#define PIOS_PWM_MAX_DEVS            1
#define PIOS_PWM_NUM_INPUTS          6

// -------------------------
// Receiver DSM input
// -------------------------
#define PIOS_DSM_MAX_DEVS            2
#define PIOS_DSM_NUM_INPUTS          12

// -------------------------
// Receiver S.Bus input
// -------------------------
#define PIOS_SBUS_MAX_DEVS           1
#define PIOS_SBUS_NUM_INPUTS         (16 + 2)

// -------------------------
// Receiver HOTT input
// -------------------------
#define PIOS_HOTT_MAX_DEVS           1
#define PIOS_HOTT_NUM_INPUTS         32

// -------------------------
// Receiver EX.Bus input
// -------------------------
#define PIOS_EXBUS_MAX_DEVS          1
#define PIOS_EXBUS_NUM_INPUTS        16

// -------------------------
// Receiver Multiplex SRXL input
// -------------------------
#define PIOS_SRXL_MAX_DEVS           1
#define PIOS_SRXL_NUM_INPUTS         16

// -------------------------
// Receiver FlySky IBus input
// -------------------------
#define PIOS_IBUS_MAX_DEVS           1
#define PIOS_IBUS_NUM_INPUTS         10

// -------------------------
// Servo outputs
// -------------------------
#define PIOS_SERVO_UPDATE_HZ         50
#define PIOS_SERVOS_INITIAL_POSITION 0 /* dont want to start motors, have no pulse till settings loaded */
#define PIOS_SERVO_BANKS             6

// --------------------------
// Timer controller settings
// --------------------------
#define PIOS_TIM_MAX_DEVS            3

// -------------------------
// GPIO
// -------------------------
#define PIOS_GPIO_PORTS              {}
#define PIOS_GPIO_PINS               {}
#define PIOS_GPIO_CLKS               {}
#define PIOS_GPIO_NUM                0

// -------------------------
// MPU6000
// -------------------------

#define PIOS_MPU6050_I2C_ADDR_AD0_LOW  0x68
#define PIOS_MPU6050_I2C_ADDR_AD0_HIGH 0x69

#define PIOS_MPU6000_I2C_ADDR          PIOS_MPU6050_I2C_ADDR_AD0_LOW

#endif /* PIOS_BOARD_H */
