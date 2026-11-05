/*
 * accel.h
 *
 *  Created on: Aug 10, 2026
 *      Author: emirhan.ucma
 */

#ifndef IIS3DWB_H_
#define IIS3DWB_H_

#include "stm32f7xx_hal.h"
#include <stdint.h>

/* Build-time choices: full scale and FIFO watermark in accelerometer samples. */
#ifndef IIS3DWB_FULL_SCALE_G
#define IIS3DWB_FULL_SCALE_G             (16)
#endif

#ifndef IIS3DWB_FIFO_WATERMARK_SAMPLES
#define IIS3DWB_FIFO_WATERMARK_SAMPLES   (64)
#endif

#if IIS3DWB_FULL_SCALE_G == 2U
#define IIS3DWB_FS_CONFIG                (0x00)
#define IIS3DWB_G_PER_LSB                (0.000061f)
#elif IIS3DWB_FULL_SCALE_G == 4U
#define IIS3DWB_FS_CONFIG                (0x08)
#define IIS3DWB_G_PER_LSB                (0.000122f)
#elif IIS3DWB_FULL_SCALE_G == 8U
#define IIS3DWB_FS_CONFIG                (0x0C)
#define IIS3DWB_G_PER_LSB                (0.000244f)
#elif IIS3DWB_FULL_SCALE_G == 16U
#define IIS3DWB_FS_CONFIG                (0x04)
#define IIS3DWB_G_PER_LSB                (0.000488f)
#else
#error "IIS3DWB_FULL_SCALE_G must be 2, 4, 8, or 16"
#endif

#if (IIS3DWB_FIFO_WATERMARK_SAMPLES < 1) || \
    (IIS3DWB_FIFO_WATERMARK_SAMPLES > 511)
#error "IIS3DWB FIFO watermark must be between 1 and 511 samples"
#endif

typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} iis3dwb_raw_data_t;

/* tag_sensor is the FIFO tag value before shifting right by three bits. */
typedef struct {
    uint8_t tag_sensor;
    iis3dwb_raw_data_t acceleration;
} iis3dwb_fifo_data_t;

#define IIS3DWB_FIFO_ACCEL_TAG            (0x02)
#define IIS3DWB_ODR_HZ                    (26667)

HAL_StatusTypeDef iis3dwb_init(void);
HAL_StatusTypeDef iis3dwb_fifo_get_status(uint16_t *sample_count,
                                           uint8_t *overrun);
HAL_StatusTypeDef iis3dwb_fifo_read(iis3dwb_fifo_data_t *data);
void iis3dwb_raw_to_g(const iis3dwb_raw_data_t *raw,
                      float *x_g, float *y_g, float *z_g);

#endif /* IIS3DWB_H_ */
