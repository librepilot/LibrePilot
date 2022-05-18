/**
 ******************************************************************************
 * @addtogroup SPRacingF3 serial BootLoader
 * @brief These files contain the code to the SPRacingF3 serial Bootloader.
 *
 * @{
 * @file       main.c
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2017.
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
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

#include <pios.h>
#include <stdbool.h>
#include <stdint.h>
#include <pios_board_info.h>
#include <op_dfu.h>
#include <pios_iap.h>
#include <fifo_buffer.h>
#include <pios_com.h>
#include <ssp.h>
#include <pios_delay.h>
#include <pios_board_init.h>
#include <pios_board_io.h>

extern void FLASH_Download();
int32_t platform_senddata(const uint8_t *msg, uint16_t msg_len);
/* Private typedef -----------------------------------------------------------*/
typedef void (*pFunction)(void);
/* Private define ------------------------------------------------------------*/
#define MAX_PACKET_DATA_LEN 255
#define MAX_PACKET_BUF_SIZE (1 + 1 + MAX_PACKET_DATA_LEN + 2)
#define UART_BUFFER_SIZE    256
#define BL_WAIT_TIME        6 * 1000 * 1000
#define DFU_BUFFER_SIZE     63

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
pFunction Jump_To_Application;
static uint32_t JumpAddress;

/// LEDs PWM
uint32_t period1 = 5000; // 5 mS
uint32_t sweep_steps1 = 100; // * 5 mS -> 500 mS
uint32_t period2 = 5000; // 5 mS
uint32_t sweep_steps2 = 100; // * 5 mS -> 500 mS

static uint8_t process_buffer[DFU_BUFFER_SIZE];
static uint8_t rx_buffer[UART_BUFFER_SIZE];
static uint8_t txBuf[MAX_PACKET_BUF_SIZE];
static uint8_t rxBuf[MAX_PACKET_BUF_SIZE];

/* Extern variables ----------------------------------------------------------*/
DFUStates DeviceState = DFUidle;
int16_t status = 0;
bool JumpToApp = false;
bool ssp_dfu   = false; // signal that ssp data has been received
bool User_DFU_request = true;


/* Private function prototypes -----------------------------------------------*/
static void led_pwm_step(uint16_t pwm_period, uint16_t pwm_sweep_steps, uint32_t stopwatch, bool default_state);
static uint32_t LedPWM(uint16_t pwm_period, uint16_t pwm_sweep_steps, uint32_t count);
static void processRX();
static void jump_to_app();


static void SSP_CallBack(uint8_t *buf, uint16_t len);
static int16_t SSP_SerialRead(void);
static void SSP_SerialWrite(uint8_t);


static const PortConfig_t ssp_portConfig = {
    .rxBuf         = rxBuf,
    .rxBufSize     = MAX_PACKET_DATA_LEN,
    .txBuf         = txBuf,
    .txBufSize     = MAX_PACKET_DATA_LEN,
    .max_retry     = 1,
    .timeoutLen    = 5000,
    .pfCallBack    = SSP_CallBack,
    .pfSerialRead  = SSP_SerialRead,
    .pfSerialWrite = SSP_SerialWrite,
    .pfGetTime     = PIOS_DELAY_GetuS,
};

static Port_t ssp_port;
static t_fifo_buffer ssp_buffer;

int main()
{
    PIOS_SYS_Init();
    PIOS_Board_Init();
    PIOS_IAP_Init();

    if (PIOS_IAP_CheckRequest() == false) {
        PIOS_DELAY_WaitmS(500);
        User_DFU_request = false;
        DeviceState = BLidle;
        PIOS_IAP_ClearRequest();
    }

    // Initialize the SSP layer between serial port and DFU
    fifoBuf_init(&ssp_buffer, rx_buffer, UART_BUFFER_SIZE);
    ssp_Init(&ssp_port, &ssp_portConfig);

    uint32_t stopwatch = 0;
    const uint32_t start_time = PIOS_DELAY_GetuS();
    while (true) {
        /* Update the stopwatch */
        stopwatch = PIOS_DELAY_GetuSSince(start_time);

        processRX();

        switch (DeviceState) {
        case Last_operation_Success:
        case uploadingStarting:
        case DFUidle:
            period1 = 5000;
            sweep_steps1 = 100;
            // PIOS_LED_Off(PIOS_LED_HEARTBEAT);
            period2 = 0;
            break;
        case uploading:
            period1 = 5000;
            sweep_steps1 = 100;
            period2 = 2500;
            sweep_steps2 = 50;
            break;
        case downloading:
            period1 = 2500;
            sweep_steps1 = 50;
            // PIOS_LED_Off(PIOS_LED_HEARTBEAT);
            period2 = 0;
            break;
        case BLidle:
            period1 = 0;
            sweep_steps1 = 100;
            PIOS_LED_On(PIOS_LED_HEARTBEAT);
            period2 = 0;
            break;
        default: // error
            period1 = 5000;
            sweep_steps1 = 100;
            period2 = 5000;
            sweep_steps2 = 100;
        }
        led_pwm_step(period1, sweep_steps1, stopwatch, false);
        led_pwm_step(period2, sweep_steps2, stopwatch, true);
        JumpToApp |= (stopwatch > BL_WAIT_TIME) && ((DeviceState == BLidle) || (DeviceState == DFUidle));
        DataDownload(start);

        if (JumpToApp == true && !ssp_dfu) {
            jump_to_app();
        }
    }
}

void led_pwm_step(uint16_t pwm_period, uint16_t pwm_sweep_steps, uint32_t stopwatch, bool default_state)
{
    if (pwm_period != 0) {
        if (LedPWM(pwm_period, pwm_sweep_steps, stopwatch)) {
            PIOS_LED_On(PIOS_LED_HEARTBEAT);
        } else {
            PIOS_LED_Off(PIOS_LED_HEARTBEAT);
        }
    } else {
        if (default_state) {
            PIOS_LED_On(PIOS_LED_HEARTBEAT);
        } else {
            PIOS_LED_Off(PIOS_LED_HEARTBEAT);
        }
    }
}
void jump_to_app()
{
    const struct pios_board_info *bdinfo = &pios_board_info_blob;

    uint32_t fwIrqStackBase = (*(__IO uint32_t *)bdinfo->fw_base) & 0xFFFE0000;

    // Check for the two possible irqstack locations (sram or core coupled sram)
    if (fwIrqStackBase == 0x20000000 || fwIrqStackBase == 0x10000000) {
        FLASH_Lock();
        RCC_APB2PeriphResetCmd(0xffffffff, ENABLE);
        RCC_APB1PeriphResetCmd(0xffffffff, ENABLE);
        RCC_APB2PeriphResetCmd(0xffffffff, DISABLE);
        RCC_APB1PeriphResetCmd(0xffffffff, DISABLE);

        JumpAddress = *(__IO uint32_t *)(bdinfo->fw_base + 4);
        Jump_To_Application = (pFunction)JumpAddress;
        /* Initialize user application's Stack Pointer */
        __set_MSP(*(__IO uint32_t *)bdinfo->fw_base);
        Jump_To_Application();
    } else {
        DeviceState = failed_jump;
        return;
    }
}

uint32_t LedPWM(uint16_t pwm_period, uint16_t pwm_sweep_steps, uint32_t count)
{
    const uint32_t curr_step  = (count / pwm_period) % pwm_sweep_steps; /* 0 - pwm_sweep_steps */
    uint32_t pwm_duty = pwm_period * curr_step / pwm_sweep_steps; /* fraction of pwm_period */

    const uint32_t curr_sweep = (count / (pwm_period * pwm_sweep_steps)); /* ticks once per full sweep */

    if (curr_sweep & 1) {
        pwm_duty = pwm_period - pwm_duty; /* reverse direction in odd sweeps */
    }
    return ((count % pwm_period) > pwm_duty) ? 1 : 0;
}
uint32_t process_count = 0;
void processRX()
{
    do {
        ssp_ReceiveProcess(&ssp_port);
        status = ssp_SendProcess(&ssp_port);
    } while ((status != SSP_TX_IDLE) && (status != SSP_TX_ACKED));

    if (fifoBuf_getUsed(&ssp_buffer) >= DFU_BUFFER_SIZE) {
        for (int32_t x = 0; x < DFU_BUFFER_SIZE; ++x) {
            process_buffer[x] = fifoBuf_getByte(&ssp_buffer);
        }
        process_count++;
        processComand(process_buffer);
    }
}
uint32_t callback_cnt = 0;
uint32_t read_cnt     = 0;
uint32_t write_cnt    = 0;
uint32_t rx_check_cnt = 0;
void SSP_CallBack(uint8_t *buf, uint16_t len)
{
    ssp_dfu = true;
    callback_cnt++;
    fifoBuf_putData(&ssp_buffer, buf, len);
}

int16_t SSP_SerialRead(void)
{
    uint8_t byte;

    rx_check_cnt++;
    if (PIOS_COM_ReceiveBuffer(PIOS_COM_TELEM_USB, &byte, 1, 0) == 1) {
        read_cnt++;
        return byte;
    } else {
        return -1;
    }
}

int32_t platform_senddata(const uint8_t *msg, uint16_t msg_len)
{
    return ssp_SendData(&ssp_port, msg, msg_len);
}

void SSP_SerialWrite(uint8_t value)
{
    write_cnt++;
    PIOS_COM_SendChar(PIOS_COM_TELEM_USB, value);
}
