/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 *
 * @file       vector_stm32f4xx.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2013
 * @brief      C based vectors for F4
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

/** interrupt handler */
typedef void (vector)(void);

/** default interrupt handler */
static void default_io_handler(void)
{
    for (;;) {
        ;
    }
}

/** prototypes an interrupt handler */
#define HANDLER(_name) extern vector _name __attribute__((weak, alias("default_io_handler")))

HANDLER(Reserved_IRQHandler); // Reserved
HANDLER(WWDG_IRQHandler); // Window WatchDog
HANDLER(PVD_IRQHandler); // PVD through EXTI Line detection
HANDLER(TAMP_STAMP_IRQHandler); // Tamper and TimeStamps through the EXTI line
HANDLER(RTC_WKUP_IRQHandler); // RTC Wakeup through the EXTI line
HANDLER(FLASH_IRQHandler); // FLASH
HANDLER(RCC_IRQHandler); // RCC
HANDLER(EXTI0_IRQHandler); // EXTI Line0
HANDLER(EXTI1_IRQHandler); // EXTI Line1
HANDLER(EXTI2_TS_IRQHandler); // EXTI Line2 and Touch Sense Interrupt
HANDLER(EXTI3_IRQHandler); // EXTI Line3
HANDLER(EXTI4_IRQHandler); // EXTI Line4
HANDLER(DMA1_Channel1_IRQHandler); // DMA1 Channel 1
HANDLER(DMA1_Channel2_IRQHandler); // DMA1 Channel 2
HANDLER(DMA1_Channel3_IRQHandler); // DMA1 Channel 3
HANDLER(DMA1_Channel4_IRQHandler); // DMA1 Channel 4
HANDLER(DMA1_Channel5_IRQHandler); // DMA1 Channel 5
HANDLER(DMA1_Channel6_IRQHandler); // DMA1 Channel 6
HANDLER(DMA1_Channel7_IRQHandler); // DMA1 Channel 7
HANDLER(ADC1_2_IRQHandler); // ADC1 and ADC2
HANDLER(USB_HP_CAN1_TX_IRQHandler); // USB Device High Priority or CAN1 TX
HANDLER(USB_LP_CAN1_RX0_IRQHandler); // USB Device Low Priority or CAN1 RX0
HANDLER(CAN1_RX1_IRQHandler); // CAN1 RX1
HANDLER(CAN1_SCE_IRQHandler); // CAN1 SCE
HANDLER(EXTI9_5_IRQHandler); // External Line[9:5]s
HANDLER(TIM1_BRK_TIM15_IRQHandler); // TIM1 Break and TIM15
HANDLER(TIM1_UP_TIM16_IRQHandler); // TIM1 Update and TIM16
HANDLER(TIM1_TRG_COM_TIM17_IRQHandler); // TIM1 Trigger and Commutation and TIM17
HANDLER(TIM1_CC_IRQHandler); // TIM1 Capture Compare
HANDLER(TIM2_IRQHandler); // TIM2
HANDLER(TIM3_IRQHandler); // TIM3
HANDLER(TIM4_IRQHandler); // TIM4
HANDLER(I2C1_EV_EXTI23_IRQHandler); // I2C1 Event and EXTI23
HANDLER(I2C1_ER_IRQHandler); // I2C1 Error
HANDLER(I2C2_EV_EXTI24_IRQHandler); // I2C2 Event and EXTI24
HANDLER(I2C2_ER_IRQHandler); // I2C2 Error
HANDLER(SPI1_IRQHandler); // SPI1
HANDLER(SPI2_IRQHandler); // SPI2
HANDLER(USART1_EXTI25_IRQHandler); // USART1 and EXTI25
HANDLER(USART2_EXTI26_IRQHandler); // USART2 and EXTI26
HANDLER(USART3_EXTI28_IRQHandler); // USART3 and EXTI28
HANDLER(EXTI15_10_IRQHandler); // External Line[15:10]s
HANDLER(RTC_Alarm_IRQHandler); // RTC Alarm (A and B) through EXTI Line
HANDLER(USB_WKUP_IRQHandler); // USB FS Wakeup through EXTI line
HANDLER(TIM8_BRK_IRQHandler); // TIM8 Break
HANDLER(TIM8_UP_IRQHandler); // TIM8 Update
HANDLER(TIM8_TRG_COM_IRQHandler); // TIM8 Trigger and Commutation
HANDLER(TIM8_CC_IRQHandler); // TIM8 Capture Compare
HANDLER(ADC3_IRQHandler); // ADC3
HANDLER(SPI3_IRQHandler); // SPI3
HANDLER(UART4_EXTI34_IRQHandler); // UART4 and EXTI34
HANDLER(UART5_EXTI35_IRQHandler); // UART5 and EXTI35
HANDLER(TIM6_DAC_IRQHandler); // TIM6 and DAC1&2 underrun errors
HANDLER(TIM7_IRQHandler); // TIM7
HANDLER(DMA2_Channel1_IRQHandler); // DMA2 Channel 1
HANDLER(DMA2_Channel2_IRQHandler); // DMA2 Channel 2
HANDLER(DMA2_Channel3_IRQHandler); // DMA2 Channel 3
HANDLER(DMA2_Channel4_IRQHandler); // DMA2 Channel 4
HANDLER(DMA2_Channel5_IRQHandler); // DMA2 Channel 5
HANDLER(ADC4_IRQHandler); // ADC4
HANDLER(COMP1_2_3_IRQHandler); // COMP1, COMP2 and COMP3
HANDLER(COMP4_5_6_IRQHandler); // COMP4, COMP5 and COMP6
HANDLER(COMP7_IRQHandler); // COMP7
HANDLER(USB_HP_IRQHandler); // USB High Priority remap
HANDLER(USB_LP_IRQHandler); // USB Low Priority remap
HANDLER(USB_WKUP_RMP_IRQHandler); // USB Wakup remap
HANDLER(FPU_IRQHandler); // FPU

/* STM32F303xE */
HANDLER(TAMPER_STAMP_IRQHandler);
HANDLER(I2C1_EV_IRQHandler);
HANDLER(USART1_IRQHandler);
HANDLER(USART2_IRQHandler);
HANDLER(USART3_IRQHandler);
HANDLER(USBWakeUp_IRQHandler);
HANDLER(FMC_IRQHandler);
HANDLER(UART4_IRQHandler);
HANDLER(UART5_IRQHandler);
HANDLER(I2C3_EV_IRQHandler);
HANDLER(I2C3_ER_IRQHandler);
HANDLER(USBWakeUp_RMP_IRQHandler);
HANDLER(TIM20_BRK_IRQHandler);
HANDLER(TIM20_UP_IRQHandler);
HANDLER(TIM20_TRG_COM_IRQHandler);
HANDLER(TIM20_CC_IRQHandler);
HANDLER(SPI4_IRQHandler);
HANDLER(I2C2_EV_IRQHandler);


/** stm32f30x interrupt vector table */
vector *io_vectors[] __attribute__((section(".io_vectors"))) = {
#ifdef STM32F303xC
    WWDG_IRQHandler,    // Window WatchDog
    PVD_IRQHandler,     // PVD through EXTI Line detection
    TAMP_STAMP_IRQHandler, // Tamper and TimeStamps through the EXTI line
    RTC_WKUP_IRQHandler, // RTC Wakeup through the EXTI line
    FLASH_IRQHandler,   // FLASH
    RCC_IRQHandler,     // RCC
    EXTI0_IRQHandler,   // EXTI Line0
    EXTI1_IRQHandler,   // EXTI Line1
    EXTI2_TS_IRQHandler, // EXTI Line2 and Touch Sense Interrupt
    EXTI3_IRQHandler,   // EXTI Line3
    EXTI4_IRQHandler,   // EXTI Line4
    DMA1_Channel1_IRQHandler, // DMA1 Channel 1
    DMA1_Channel2_IRQHandler, // DMA1 Channel 2
    DMA1_Channel3_IRQHandler, // DMA1 Channel 3
    DMA1_Channel4_IRQHandler, // DMA1 Channel 4
    DMA1_Channel5_IRQHandler, // DMA1 Channel 5
    DMA1_Channel6_IRQHandler, // DMA1 Channel 6
    DMA1_Channel7_IRQHandler, // DMA1 Channel 7
    ADC1_2_IRQHandler,  // ADC1 and ADC2
    USB_HP_CAN1_TX_IRQHandler, // USB Device High Priority or CAN1 TX
    USB_LP_CAN1_RX0_IRQHandler, // USB Device Low Priority or CAN1 RX0
    CAN1_RX1_IRQHandler, // CAN1 RX1
    CAN1_SCE_IRQHandler, // CAN1 SCE
    EXTI9_5_IRQHandler, // External Line[9:5]s
    TIM1_BRK_TIM15_IRQHandler, // TIM1 Break and TIM15
    TIM1_UP_TIM16_IRQHandler, // TIM1 Update and TIM16
    TIM1_TRG_COM_TIM17_IRQHandler, // TIM1 Trigger and Commutation and TIM17
    TIM1_CC_IRQHandler, // TIM1 Capture Compare
    TIM2_IRQHandler,    // TIM2
    TIM3_IRQHandler,    // TIM3
    TIM4_IRQHandler,    // TIM4
    I2C1_EV_EXTI23_IRQHandler, // I2C1 Event and EXTI23
    I2C1_ER_IRQHandler, // I2C1 Error
    I2C2_EV_EXTI24_IRQHandler, // I2C2 Event and EXTI24
    I2C2_ER_IRQHandler, // I2C2 Error
    SPI1_IRQHandler,    // SPI1
    SPI2_IRQHandler,    // SPI2
    USART1_EXTI25_IRQHandler, // USART1 and EXTI25
    USART2_EXTI26_IRQHandler, // USART2 and EXTI26
    USART3_EXTI28_IRQHandler, // USART3 and EXTI28
    EXTI15_10_IRQHandler, // External Line[15:10]s
    RTC_Alarm_IRQHandler, // RTC Alarm (A and B) through EXTI Line
    USB_WKUP_IRQHandler, // USB FS Wakeup through EXTI line
    TIM8_BRK_IRQHandler, // TIM8 Break
    TIM8_UP_IRQHandler, // TIM8 Update
    TIM8_TRG_COM_IRQHandler, // TIM8 Trigger and Commutation
    TIM8_CC_IRQHandler, // TIM8 Capture Compare
    ADC3_IRQHandler,    // ADC3
    Reserved_IRQHandler, // reserved
    Reserved_IRQHandler, // reserved
    Reserved_IRQHandler, // reserved
    SPI3_IRQHandler,    // SPI3
    UART4_EXTI34_IRQHandler, // UART4 and EXTI34
    UART5_EXTI35_IRQHandler, // UART5 and EXTI35
    TIM6_DAC_IRQHandler, // TIM6 and DAC1&2 underrun errors
    TIM7_IRQHandler,    // TIM7
    DMA2_Channel1_IRQHandler, // DMA2 Channel 1
    DMA2_Channel2_IRQHandler, // DMA2 Channel 2
    DMA2_Channel3_IRQHandler, // DMA2 Channel 3
    DMA2_Channel4_IRQHandler, // DMA2 Channel 4
    DMA2_Channel5_IRQHandler, // DMA2 Channel 5
    ADC4_IRQHandler,    // ADC4
    Reserved_IRQHandler, // reserved
    Reserved_IRQHandler, // reserved
    COMP1_2_3_IRQHandler, // COMP1, COMP2 and COMP3
    COMP4_5_6_IRQHandler, // COMP4, COMP5 and COMP6
    COMP7_IRQHandler,   // COMP7
    Reserved_IRQHandler, // reserved
    Reserved_IRQHandler, // reserved
    Reserved_IRQHandler, // reserved
    Reserved_IRQHandler, // reserved
    Reserved_IRQHandler, // reserved
    Reserved_IRQHandler, // reserved
    Reserved_IRQHandler, // reserved
    USB_HP_IRQHandler,  // USB High Priority remap
    USB_LP_IRQHandler,  // USB Low Priority remap
    USB_WKUP_RMP_IRQHandler, // USB Wakup remap
    Reserved_IRQHandler, // reserved
    Reserved_IRQHandler, // reserved
    Reserved_IRQHandler, // reserved
    Reserved_IRQHandler, // reserved
    FPU_IRQHandler,     // FPU
#endif /* ifdef STM32F303xC */
#ifdef STM32F303xE
    WWDG_IRQHandler,    /*!< Window WatchDog Interrupt                                         */
    PVD_IRQHandler,     /*!< PVD through EXTI Line detection Interrupt                         */
    TAMPER_STAMP_IRQHandler, /*!< Tamper and TimeStamp interrupts                                   */
    RTC_WKUP_IRQHandler, /*!< RTC Wakeup interrupt through the EXTI lines 17, 19 & 20           */
    FLASH_IRQHandler,   /*!< FLASH global Interrupt                                            */
    RCC_IRQHandler,     /*!< RCC global Interrupt                                              */
    EXTI0_IRQHandler,   /*!< EXTI Line0 Interrupt                                              */
    EXTI1_IRQHandler,   /*!< EXTI Line1 Interrupt                                              */
    EXTI2_TS_IRQHandler, /*!< EXTI Line2 Interrupt and Touch Sense Interrupt                    */
    EXTI3_IRQHandler,   /*!< EXTI Line3 Interrupt                                              */
    EXTI4_IRQHandler,   /*!< EXTI Line4 Interrupt                                              */
    DMA1_Channel1_IRQHandler, /*!< DMA1 Channel 1 Interrupt                                          */
    DMA1_Channel2_IRQHandler, /*!< DMA1 Channel 2 Interrupt                                          */
    DMA1_Channel3_IRQHandler, /*!< DMA1 Channel 3 Interrupt                                          */
    DMA1_Channel4_IRQHandler, /*!< DMA1 Channel 4 Interrupt                                          */
    DMA1_Channel5_IRQHandler, /*!< DMA1 Channel 5 Interrupt                                          */
    DMA1_Channel6_IRQHandler, /*!< DMA1 Channel 6 Interrupt                                          */
    DMA1_Channel7_IRQHandler, /*!< DMA1 Channel 7 Interrupt                                          */
    ADC1_2_IRQHandler,  /*!< ADC1 & ADC2 Interrupts                                            */
    USB_HP_CAN1_TX_IRQHandler, /*!< USB Device High Priority or CAN1 TX Interrupts                    */
    USB_LP_CAN1_RX0_IRQHandler, /*!< USB Device Low Priority or CAN1 RX0 Interrupts                    */
    CAN1_RX1_IRQHandler, /*!< CAN1 RX1 Interrupt                                                */
    CAN1_SCE_IRQHandler, /*!< CAN1 SCE Interrupt                                                */
    EXTI9_5_IRQHandler, /*!< External Line[9:5] Interrupts                                     */
    TIM1_BRK_TIM15_IRQHandler, /*!< TIM1 Break and TIM15 Interrupts                                   */
    TIM1_UP_TIM16_IRQHandler, /*!< TIM1 Update and TIM16 Interrupts                                  */
    TIM1_TRG_COM_TIM17_IRQHandler, /*!< TIM1 Trigger and Commutation and TIM17 Interrupt                  */
    TIM1_CC_IRQHandler, /*!< TIM1 Capture Compare Interrupt                                    */
    TIM2_IRQHandler,    /*!< TIM2 global Interrupt                                             */
    TIM3_IRQHandler,    /*!< TIM3 global Interrupt                                             */
    TIM4_IRQHandler,    /*!< TIM4 global Interrupt                                             */
    I2C1_EV_IRQHandler, /*!< I2C1 Event Interrupt                                              */
    I2C1_ER_IRQHandler, /*!< I2C1 Error Interrupt                                              */
    I2C2_EV_IRQHandler, /*!< I2C2 Event Interrupt                                              */
    I2C2_ER_IRQHandler, /*!< I2C2 Error Interrupt                                              */
    SPI1_IRQHandler,    /*!< SPI1 global Interrupt                                             */
    SPI2_IRQHandler,    /*!< SPI2 global Interrupt                                             */
    USART1_IRQHandler,  /*!< USART1 global Interrupt                                           */
    USART2_IRQHandler,  /*!< USART2 global Interrupt                                           */
    USART3_IRQHandler,  /*!< USART3 global Interrupt                                           */
    EXTI15_10_IRQHandler, /*!< External Line[15:10] Interrupts                                   */
    RTC_Alarm_IRQHandler, /*!< RTC Alarm (A and B) through EXTI Line Interrupt                   */
    USBWakeUp_IRQHandler, /*!< USB Wakeup Interrupt                                              */
    TIM8_BRK_IRQHandler, /*!< TIM8 Break Interrupt                                              */
    TIM8_UP_IRQHandler, /*!< TIM8 Update Interrupt                                             */
    TIM8_TRG_COM_IRQHandler, /*!< TIM8 Trigger and Commutation Interrupt                            */
    TIM8_CC_IRQHandler, /*!< TIM8 Capture Compare Interrupt                                    */
    ADC3_IRQHandler,    /*!< ADC3 global Interrupt                                             */
    FMC_IRQHandler,     /*!< FMC global Interrupt                                              */
    Reserved_IRQHandler,
    Reserved_IRQHandler,
    SPI3_IRQHandler,    /*!< SPI3 global Interrupt                                             */
    UART4_IRQHandler,   /*!< UART4 global Interrupt                                            */
    UART5_IRQHandler,   /*!< UART5 global Interrupt                                            */
    TIM6_DAC_IRQHandler, /*!< TIM6 global and DAC1&2 underrun error  interrupts                 */
    TIM7_IRQHandler,    /*!< TIM7 global Interrupt                                             */
    DMA2_Channel1_IRQHandler, /*!< DMA2 Channel 1 global Interrupt                                   */
    DMA2_Channel2_IRQHandler, /*!< DMA2 Channel 2 global Interrupt                                   */
    DMA2_Channel3_IRQHandler, /*!< DMA2 Channel 3 global Interrupt                                   */
    DMA2_Channel4_IRQHandler, /*!< DMA2 Channel 4 global Interrupt                                   */
    DMA2_Channel5_IRQHandler, /*!< DMA2 Channel 5 global Interrupt                                   */
    ADC4_IRQHandler,    /*!< ADC4  global Interrupt                                            */
    Reserved_IRQHandler,
    Reserved_IRQHandler,
    COMP1_2_3_IRQHandler, /*!< COMP1, COMP2 and COMP3 global Interrupt                           */
    COMP4_5_6_IRQHandler, /*!< COMP5, COMP6 and COMP4 global Interrupt                           */
    COMP7_IRQHandler,   /*!< COMP7 global Interrupt                                            */
    Reserved_IRQHandler,
    Reserved_IRQHandler,
    Reserved_IRQHandler,
    Reserved_IRQHandler,
    Reserved_IRQHandler,
    I2C3_EV_IRQHandler, /*!< I2C3 event interrupt                                              */
    I2C3_ER_IRQHandler, /*!< I2C3 error interrupt                                              */
    USB_HP_IRQHandler,  /*!< USB High Priority global Interrupt remap                          */
    USB_LP_IRQHandler,  /*!< USB Low Priority global Interrupt  remap                          */
    USBWakeUp_RMP_IRQHandler, /*!< USB Wakeup Interrupt remap                                        */
    TIM20_BRK_IRQHandler, /*!< TIM20 Break Interrupt                                             */
    TIM20_UP_IRQHandler, /*!< TIM20 Update Interrupt                                            */
    TIM20_TRG_COM_IRQHandler, /*!< TIM20 Trigger and Commutation Interrupt                           */
    TIM20_CC_IRQHandler, /*!< TIM20 Capture Compare Interrupt                                   */
    FPU_IRQHandler,     /*!< Floating point Interrupt                                          */
    Reserved_IRQHandler,
    Reserved_IRQHandler,
    SPI4_IRQHandler /*!< SPI4 global Interrupt                                             */
#endif /* ifdef STM32F303xE */
};

/**
 * @}
 */
