/*
 * accel.c
 *
 *  Created on: Aug 10, 2026
 *      Author: emirhan.ucma
 */

#include "iis3dwb.h"
#include "spi_drv.h"
#include "spi.h"
#include <stddef.h>

#define IIS3DWB_REG_FIFO_CTRL1      		(0x07)
#define IIS3DWB_REG_FIFO_CTRL2       		(0x08)
#define IIS3DWB_REG_FIFO_CTRL3       		(0x09)
#define IIS3DWB_REG_FIFO_CTRL4       		(0x0A)
#define IIS3DWB_REG_INT1_CTRL        		(0x0D)
#define IIS3DWB_REG_WHO_AM_I         		(0x0F)
#define IIS3DWB_REG_CTRL1_XL         		(0x10)
#define IIS3DWB_REG_CTRL3_C           		(0x12)
#define IIS3DWB_REG_FIFO_STATUS1     		(0x3A)
#define IIS3DWB_REG_FIFO_STATUS2      		(0x3B)
#define IIS3DWB_REG_FIFO_DATA_OUT_TAG 		(0x78)

#define IIS3DWB_WHO_AM_I_VALUE        		(0x7B)
#define IIS3DWB_SPI_READ_MASK        		(0x80)
#define IIS3DWB_CTRL3_BDU             		(1 << 6)
#define IIS3DWB_CTRL3_IF_INC          		(1 << 2)
#define IIS3DWB_CTRL1_XL_ENABLE				(0xA0)
#define IIS3DWB_CTRL1_FS_MASK         		(0x0C)
#define IIS3DWB_FIFO_BDR_XL_26K7      		(0x0A)
#define IIS3DWB_FIFO_MODE_BYPASS      		(0x00)
#define IIS3DWB_FIFO_MODE_CONTINUOUS  		(0x06)
#define IIS3DWB_INT1_FIFO_WTM         		(1 << 3)
#define IIS3DWB_INT1_FIFO_OVR         		(1 << 4)
#define IIS3DWB_FIFO_OVR_STATUS       		(1 << 6)
#define IIS3DWB_FIFO_COUNT_HIGH_MASK  		(0x03)
#define IIS3DWB_FIFO_SAMPLE_BYTES     		(7)

SPI_DEVICE_DECLARE(iis3dwb_spi, hspi1, SPI_CS_GPIO_Port, SPI_CS_Pin);

static HAL_StatusTypeDef iis3dwb_write_register(uint8_t reg, uint8_t value)
{
    uint8_t tx[2] = {reg, value};

    return spi_write(&iis3dwb_spi, tx, (uint16_t)sizeof(tx));
}

static HAL_StatusTypeDef iis3dwb_read_register(uint8_t reg, uint8_t *value)
{
    uint8_t tx[2] = {(uint8_t)(reg | IIS3DWB_SPI_READ_MASK), 0};
    uint8_t rx[2] = {0, 0};
    HAL_StatusTypeDef status;

    if (value == NULL) {
        return HAL_ERROR;
    }

    status = spi_read(&iis3dwb_spi, tx, rx, (uint16_t)sizeof(tx));
    if (status == HAL_OK) {
        *value = rx[1];
    }

    return status;
}

HAL_StatusTypeDef iis3dwb_init(void)
{
    uint8_t who_am_i;
    uint8_t ctrl3_c;
    uint8_t ctrl1_xl;
    HAL_StatusTypeDef status;
    const uint16_t watermark = (uint16_t)IIS3DWB_FIFO_WATERMARK_SAMPLES;

    status = iis3dwb_read_register(IIS3DWB_REG_WHO_AM_I, &who_am_i);
    if (status != HAL_OK) {
        return status;
    }
    if (who_am_i != IIS3DWB_WHO_AM_I_VALUE) {
        return HAL_ERROR;
    }

    status = iis3dwb_read_register(IIS3DWB_REG_CTRL3_C, &ctrl3_c);
    if (status != HAL_OK) {
        return status;
    }
    ctrl3_c |= (uint8_t)(IIS3DWB_CTRL3_BDU | IIS3DWB_CTRL3_IF_INC);
    status = iis3dwb_write_register(IIS3DWB_REG_CTRL3_C, ctrl3_c);
    if (status != HAL_OK) {
        return status;
    }

    status = iis3dwb_write_register(IIS3DWB_REG_FIFO_CTRL4, IIS3DWB_FIFO_MODE_BYPASS);
    if (status != HAL_OK) {
        return status;
    }

    status = iis3dwb_read_register(IIS3DWB_REG_CTRL1_XL, &ctrl1_xl);
    if (status != HAL_OK) {
        return status;
    }
    ctrl1_xl &= (uint8_t)~IIS3DWB_CTRL1_FS_MASK;
    ctrl1_xl = (uint8_t)((ctrl1_xl & 0x0F) | IIS3DWB_CTRL1_XL_ENABLE | IIS3DWB_FS_CONFIG);
    status = iis3dwb_write_register(IIS3DWB_REG_CTRL1_XL, ctrl1_xl);

    if (status != HAL_OK) {
        return status;
    }

    status = iis3dwb_write_register(IIS3DWB_REG_FIFO_CTRL1, (uint8_t)(watermark & 0xFF));
    if (status != HAL_OK) {
        return status;
    }

    status = iis3dwb_write_register(IIS3DWB_REG_FIFO_CTRL2, (uint8_t)((watermark >> 8) & 0x01));
    if (status != HAL_OK) {
        return status;
    }
    status = iis3dwb_write_register(IIS3DWB_REG_FIFO_CTRL3, IIS3DWB_FIFO_BDR_XL_26K7);
    if (status != HAL_OK) {
        return status;
    }
    status = iis3dwb_write_register(IIS3DWB_REG_FIFO_CTRL4, IIS3DWB_FIFO_MODE_CONTINUOUS);
    if (status != HAL_OK) {
        return status;
    }

    return iis3dwb_write_register(IIS3DWB_REG_INT1_CTRL, (uint8_t)(IIS3DWB_INT1_FIFO_WTM | IIS3DWB_INT1_FIFO_OVR));
}

HAL_StatusTypeDef iis3dwb_fifo_get_status(uint16_t *sample_count, uint8_t *overrun)
{
    uint8_t status1;
    uint8_t status2;
    HAL_StatusTypeDef status;

    if ((sample_count == NULL) || (overrun == NULL)) {
        return HAL_ERROR;
    }

    status = iis3dwb_read_register(IIS3DWB_REG_FIFO_STATUS1, &status1);
    if (status != HAL_OK) {
        return status;
    }
    status = iis3dwb_read_register(IIS3DWB_REG_FIFO_STATUS2, &status2);
    if (status != HAL_OK) {
        return status;
    }

    *sample_count = (uint16_t)status1 | (uint16_t)(((uint16_t)status2 & IIS3DWB_FIFO_COUNT_HIGH_MASK) << 8);
    *overrun = (uint8_t)((status2 & IIS3DWB_FIFO_OVR_STATUS) != 0);
    return HAL_OK;
}

HAL_StatusTypeDef iis3dwb_fifo_read(iis3dwb_fifo_data_t *data)
{
    uint8_t tx[IIS3DWB_FIFO_SAMPLE_BYTES + 1] = {0};
    uint8_t rx[IIS3DWB_FIFO_SAMPLE_BYTES + 1] = {0};
    HAL_StatusTypeDef status;

    if (data == NULL) {
        return HAL_ERROR;
    }

    tx[0] = (uint8_t)(IIS3DWB_REG_FIFO_DATA_OUT_TAG | IIS3DWB_SPI_READ_MASK);

    status = spi_read(&iis3dwb_spi, tx, rx, (uint16_t)sizeof(tx));
    if (status != HAL_OK) {
        return status;
    }

    data->tag_sensor = (uint8_t)(rx[1] >> 3);

    data->acceleration.x = (int16_t)((uint16_t)rx[2] | ((uint16_t)rx[3] << 8));
    data->acceleration.y = (int16_t)((uint16_t)rx[4] | ((uint16_t)rx[5] << 8));
    data->acceleration.z = (int16_t)((uint16_t)rx[6] | ((uint16_t)rx[7] << 8));

    return HAL_OK;
}

void iis3dwb_raw_to_g(const iis3dwb_raw_data_t *raw, float *x_g, float *y_g, float *z_g)
{
    if ((raw == NULL) || (x_g == NULL) || (y_g == NULL) || (z_g == NULL)) {
        return;
    }

    *x_g = (float)raw->x * IIS3DWB_G_PER_LSB;
    *y_g = (float)raw->y * IIS3DWB_G_PER_LSB;
    *z_g = (float)raw->z * IIS3DWB_G_PER_LSB;
}
