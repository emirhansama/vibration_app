/*
 * spi_drv.h
 *
 *  Created on: Aug 11, 2026
 *      Author: emirhan.ucma
 */

#ifndef DRV_SPI_SPI_DRV_H_
#define DRV_SPI_SPI_DRV_H_

#include "main.h"

/**
 * @brief SPI device configuration structure.
 *
 * Contains the SPI peripheral handle and chip select
 * GPIO information required to communicate with an SPI device.
 */
typedef struct
{
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef *cs_port;
    uint16_t cs_pin;

} SpiDevice_t;

/**
 * @def SPI_DEVICE_DECLARE
 * @brief Declares and initializes an SPI device.
 *
 * Creates a static SPI device structure containing the SPI handle
 * and chip select GPIO configuration.
 *
 * @param name Device variable name.
 * @param spi_handle SPI handle.
 * @param port CS GPIO port.
 * @param pin CS GPIO pin.
 */
#define SPI_DEVICE_DECLARE(name, spi_handle, port, pin) \
    static SpiDevice_t name =                          \
    {                                                  \
        .hspi = &(spi_handle),                         \
        .cs_port = (port),                             \
        .cs_pin = (pin)                                \
    }

/**
 * @brief Transmits data to an SPI device.
 *
 * Pulls chip select low, transmits the given data buffer,
 * then releases chip select.
 *
 * @param device Pointer to the SPI device structure.
 * @param data Pointer to the data buffer to be transmitted.
 * @param len Number of bytes to transmit.
 *
 * @return HAL status of the SPI transmission.
 */
HAL_StatusTypeDef spi_write(SpiDevice_t *device, uint8_t *data, uint16_t len);

/**
 * @brief Performs a full-duplex SPI transfer.
 *
 * Pulls chip select low, transmits data from the TX buffer
 * while receiving data into the RX buffer, then releases chip select.
 *
 * @param device Pointer to the SPI device structure.
 * @param tx_data Pointer to the transmit data buffer.
 * @param rx_data Pointer to the receive data buffer.
 * @param len Number of bytes to transfer.
 *
 * @return HAL status of the SPI transfer.
 */
HAL_StatusTypeDef spi_read(SpiDevice_t *device, uint8_t *tx_data, uint8_t *rx_data, uint16_t len);

#endif /* DRV_SPI_SPI_DRV_H_ */
