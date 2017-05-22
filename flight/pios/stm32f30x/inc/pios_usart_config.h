/*
 * pios_usart_config.h
 */

#ifndef PIOS_USART_CONFIG_H_
#define PIOS_USART_CONFIG_H_

/**
 * Generic USART configuration structure for an STM32F30x port.
 */
#define USART_CONFIG(_usart, _rx_gpio, _rx_pin, _tx_gpio, _tx_pin) \
    {                                                                       \
        .regs  = _usart,                                                    \
        .remap = GPIO_AF_##_usart,                                        \
        .rx = {                                                           \
            .gpio = GPIO##_rx_gpio,                                               \
            .init = {                                                       \
                .GPIO_Pin   = GPIO_Pin_##_rx_pin,                                      \
                .GPIO_Mode  = GPIO_Mode_AF,                                 \
                .GPIO_Speed = GPIO_Speed_50MHz,                             \
                .GPIO_OType = GPIO_OType_PP,                                \
                .GPIO_PuPd  = GPIO_PuPd_UP,                                 \
            },                                                              \
            .pin_source     = GPIO_PinSource##_rx_pin,       \
        },                                                                  \
        .tx = {                                                           \
            .gpio = GPIO##_tx_gpio,                                               \
            .init = {                                                       \
                .GPIO_Pin   = GPIO_Pin_##_tx_pin,                                      \
                .GPIO_Mode  = GPIO_Mode_AF,                                 \
                .GPIO_Speed = GPIO_Speed_50MHz,                             \
                .GPIO_OType = GPIO_OType_PP,                                \
                .GPIO_PuPd  = GPIO_PuPd_UP,                                 \
            },                                                              \
            .pin_source     = GPIO_PinSource##_tx_pin,       \
        },                                                                  \
    }

#endif /* PIOS_USART_CONFIG_H_ */
