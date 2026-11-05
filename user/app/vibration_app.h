/*
 * vibration_app.h
 *
 *  Created on: Sep 30, 2026
 *      Author: emirhan.ucma
 */

#ifndef APP_VIBRATION_APP_H_
#define APP_VIBRATION_APP_H_

#include "main.h"
#include "cmsis_os2.h"
#include "lwip/api.h"
#include "lwip.h"
#include "iis3dwb.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

void StartDefaultTask(void *argument);
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

#endif /* APP_VIBRATION_APP_H_ */
