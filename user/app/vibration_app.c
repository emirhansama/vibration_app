/*
 * vibration_app.c
 *
 * Created on: Sep 30, 2026
 * Author: emirhan.ucma
 */

#include "vibration_app.h"
#include "FreeRTOS.h"
#include "spi.h"
#include <stddef.h>
#include <string.h>

#define IIS3DWB_INIT_RETRY_DELAY_MS        (100)
#define IIS3DWB_STARTUP_SETTLE_DELAY_MS    (10)
#define IIS3DWB_FIFO_BATCH_SIZE            (64)

#define SAMPLE_QUEUE_LENGTH                (8)

#define ETHERNET_START_DELAY_MS            (200)
#define ETHERNET_ERROR_DELAY_MS            (1000)
#define DEBUG_TCP_PORT                     (5000)

#define SERVER_IP_1                        (192)
#define SERVER_IP_2                        (168)
#define SERVER_IP_3                        (1)
#define SERVER_IP_4                        (50)

#define IIS3DWB_FIFO_WATERMARK_FLAG        (1 << 0)

#define TCP_FRAME_MAGIC                    (0xA55A)
#define TCP_FRAME_VERSION                  (1)


typedef struct {
    uint32_t first_sample_index;
    uint32_t fifo_overrun_count;
    uint32_t queue_drop_count;
    uint16_t sample_count;
    int16_t xyz[IIS3DWB_FIFO_BATCH_SIZE][3];
} AccelBatch_t;


typedef struct __attribute__((packed)) {
    uint16_t magic;
    uint16_t frame_size;
    uint16_t version;
    uint16_t sample_count;
    uint32_t first_sample_index;
    uint32_t fifo_overrun_count;
    uint32_t queue_drop_count;
    int16_t xyz[IIS3DWB_FIFO_BATCH_SIZE][3];
} TcpAccelFrame_t;


static osThreadId_t vibrationTaskHandle;
static osThreadId_t ethernetTaskHandle;
static osThreadId_t ledTaskHandle;

static osMessageQueueId_t accelQueueHandle;

static volatile uint32_t queue_drop_count;


static const osThreadAttr_t vibrationTaskAttributes = {
    .name = "VibrationTask",
    .stack_size = 2048,
    .priority = osPriorityLow
};

static const osThreadAttr_t ethernetTaskAttributes = {
    .name = "EthernetTask",
    .stack_size = 2048,
    .priority = osPriorityLow
};

static const osThreadAttr_t ledTaskAttributes = {
    .name = "LedTask",
    .stack_size = 256,
    .priority = osPriorityLow
};


static void LedTask(void *argument);
static void VibrationTask(void *argument);
static void EthernetTask(void *argument);


void StartDefaultTask(void *argument)
{
    osDelay(2000);

    __HAL_RCC_ETHMAC_FORCE_RESET();
    osDelay(10);
    __HAL_RCC_ETHMAC_RELEASE_RESET();
    osDelay(100);

    MX_LWIP_Init();

    accelQueueHandle = osMessageQueueNew(SAMPLE_QUEUE_LENGTH, sizeof(AccelBatch_t), NULL);

    if (accelQueueHandle == NULL) {
        Error_Handler();
    }

    vibrationTaskHandle = osThreadNew(VibrationTask, NULL, &vibrationTaskAttributes);
    ethernetTaskHandle = osThreadNew(EthernetTask, NULL, &ethernetTaskAttributes);
    ledTaskHandle = osThreadNew(LedTask, NULL, &ledTaskAttributes);

    if ((vibrationTaskHandle == NULL) || (ethernetTaskHandle == NULL) || (ledTaskHandle == NULL)) {
        Error_Handler();
    }

    for (;;) {
        osDelay(1000);
    }
}


static void LedTask(void *argument)
{
    for (;;) {
        HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
        osDelay(500);
    }
}


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if ((GPIO_Pin == GPIO_PIN_0) && (vibrationTaskHandle != NULL)) {
        (void)osThreadFlagsSet(vibrationTaskHandle, IIS3DWB_FIFO_WATERMARK_FLAG);
    }
}


static void VibrationTask(void *argument)
{
    static AccelBatch_t batch;

    uint32_t next_sample_index = 0;
    uint32_t fifo_overrun_count = 0;

    uint8_t overrun_was_set = 0;

    for (;;) {
        HAL_StatusTypeDef init_status = iis3dwb_init();

        if (init_status == HAL_OK) {
            break;
        }

        osDelay(IIS3DWB_INIT_RETRY_DELAY_MS);
    }

    osDelay(IIS3DWB_STARTUP_SETTLE_DELAY_MS);

    for (;;) {
        uint32_t flags = osThreadFlagsWait(IIS3DWB_FIFO_WATERMARK_FLAG, osFlagsWaitAny, osWaitForever);

        if ((flags & osFlagsError) != 0) {
            continue;
        }

        for (;;) {
            uint16_t fifo_words;
            uint16_t words_to_read;
            uint16_t word_index;

            uint8_t overrun;
            uint8_t read_failed = 0;

            HAL_StatusTypeDef status;

            status = iis3dwb_fifo_get_status(&fifo_words, &overrun);

            if (status != HAL_OK) {
                osDelay(1);
                (void)osThreadFlagsSet(vibrationTaskHandle, IIS3DWB_FIFO_WATERMARK_FLAG);
                break;
            }

            if ((overrun != 0) && (overrun_was_set == 0)) {
                fifo_overrun_count++;
            }

            overrun_was_set = overrun;

            if (fifo_words == 0) {
                break;
            }

            words_to_read = fifo_words;

            if (words_to_read > IIS3DWB_FIFO_BATCH_SIZE) {
                words_to_read = IIS3DWB_FIFO_BATCH_SIZE;
            }

            batch.sample_count = 0;
            batch.first_sample_index = next_sample_index;
            batch.fifo_overrun_count = fifo_overrun_count;
            batch.queue_drop_count = queue_drop_count;

            for (word_index = 0; word_index < words_to_read; word_index++) {
                iis3dwb_fifo_data_t fifo_data;

                status = iis3dwb_fifo_read(&fifo_data);

                if (status != HAL_OK) {
                    read_failed = 1;
                    break;
                }

                if (fifo_data.tag_sensor == IIS3DWB_FIFO_ACCEL_TAG) {
                    uint16_t sample = batch.sample_count;

                    batch.xyz[sample][0] = fifo_data.acceleration.x;
                    batch.xyz[sample][1] = fifo_data.acceleration.y;
                    batch.xyz[sample][2] = fifo_data.acceleration.z;

                    batch.sample_count++;
                    next_sample_index++;
                }
            }

            if (read_failed != 0) {
                osDelay(1);
                (void)osThreadFlagsSet(vibrationTaskHandle, IIS3DWB_FIFO_WATERMARK_FLAG);
                break;
            }

            if (batch.sample_count != 0) {
                osStatus_t queue_status = osMessageQueuePut(accelQueueHandle, &batch, 0, 0);

                if (queue_status != osOK) {
                    queue_drop_count++;
                }
            }
        }
    }
}


static void EthernetTask(void *argument)
{
    struct netconn *server;
    struct netconn *client;

    ip4_addr_t server_ip;

    err_t err;

    static AccelBatch_t batch;
    static TcpAccelFrame_t frame;

    osDelay(ETHERNET_START_DELAY_MS);

    server = netconn_new(NETCONN_TCP);

    if (server == NULL) {
        for (;;) {
            osDelay(ETHERNET_ERROR_DELAY_MS);
        }
    }

    IP4_ADDR(&server_ip, SERVER_IP_1, SERVER_IP_2, SERVER_IP_3, SERVER_IP_4);

    err = netconn_bind(server, &server_ip, DEBUG_TCP_PORT);

    if (err != ERR_OK) {
        netconn_delete(server);

        for (;;) {
            osDelay(ETHERNET_ERROR_DELAY_MS);
        }
    }

    err = netconn_listen(server);

    if (err != ERR_OK) {
        netconn_delete(server);

        for (;;) {
            osDelay(ETHERNET_ERROR_DELAY_MS);
        }
    }

    for (;;) {
        err = netconn_accept(server, &client);

        if (err != ERR_OK) {
            osDelay(ETHERNET_ERROR_DELAY_MS);
            continue;
        }

        for (;;) {
            osStatus_t queue_status;
            size_t payload_size;
            size_t frame_size;

            queue_status = osMessageQueueGet(accelQueueHandle, &batch, NULL, osWaitForever);

            if (queue_status != osOK) {
                continue;
            }

            frame.magic = TCP_FRAME_MAGIC;
            frame.version = TCP_FRAME_VERSION;
            frame.sample_count = batch.sample_count;
            frame.first_sample_index = batch.first_sample_index;
            frame.fifo_overrun_count = batch.fifo_overrun_count;
            frame.queue_drop_count = batch.queue_drop_count;

            payload_size = batch.sample_count * 3 * sizeof(int16_t);

            memcpy(frame.xyz, batch.xyz, payload_size);

            frame_size = offsetof(TcpAccelFrame_t, xyz) + payload_size;
            frame.frame_size = (uint16_t)frame_size;

            err = netconn_write(client, &frame, frame_size, NETCONN_COPY);

            if (err != ERR_OK) {
                break;
            }
        }

        netconn_close(client);
        netconn_delete(client);
    }
}
