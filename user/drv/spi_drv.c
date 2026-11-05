/*
 * spi_drv.c
 *
 *  Created on: Aug 11, 2026
 *      Author: emirhan.ucma
 */

#include "spi_drv.h"

/**
 * @brief Transmits data to an SPI device.
 *
 * Pulls the device chip select low, transmits the given data buffer
 * over SPI, then releases chip select.
 *
 * @param device Pointer to the SPI device structure.
 * @param data Pointer to the data buffer to be transmitted.
 * @param len Number of bytes to transmit.
 *
 * @return HAL status of the SPI transmission.
 */
HAL_StatusTypeDef spi_write(SpiDevice_t *device, uint8_t *data, uint16_t len)
{
    HAL_StatusTypeDef status;

    HAL_GPIO_WritePin(device->cs_port, device->cs_pin, GPIO_PIN_RESET);

    status = HAL_SPI_Transmit(device->hspi, data, len, 10);

    HAL_GPIO_WritePin(device->cs_port, device->cs_pin, GPIO_PIN_SET);

    return status;
}

/**
 * @brief Performs a full-duplex SPI transfer.
 *
 * Pulls the device chip select low, transmits data from the TX buffer
 * while receiving data into the RX buffer, then releases chip select.
 *
 * @param device Pointer to the SPI device structure.
 * @param tx_data Pointer to the transmit data buffer.
 * @param rx_data Pointer to the receive data buffer.
 * @param len Number of bytes to transfer.
 *
 * @return HAL status of the SPI transfer.
 */
HAL_StatusTypeDef spi_read(SpiDevice_t *device, uint8_t *tx_data, uint8_t *rx_data, uint16_t len)
{
    HAL_StatusTypeDef status;

    HAL_GPIO_WritePin(device->cs_port, device->cs_pin, GPIO_PIN_RESET);

    status = HAL_SPI_TransmitReceive(device->hspi, tx_data, rx_data, len, 10);

    HAL_GPIO_WritePin(device->cs_port, device->cs_pin, GPIO_PIN_SET);

    return status;
}
