#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "A7169.h"

#define A7169_GIO1_IRQ_PIN  GPIO_NUM_10

static const char *TAG = "main";
static TaskHandle_t s_rf_task = NULL;

static void IRAM_ATTR gpio10_isr_handler(void *arg)
{
    if (s_rf_task) {
        BaseType_t woken = pdFALSE;
        xTaskNotifyFromISR(s_rf_task, 1, eSetValueWithOverwrite, &woken);
        portYIELD_FROM_ISR(woken);
    }
}

static void rf_recv_task(void *pv)
{
    uint8_t buf[64];
    uint32_t val;
    rf_normal_data_t data;

    ESP_LOGI(TAG, "433 RF 接收监听中...");

    while (1) {
        if (xTaskNotifyWait(0, 0, &val, pdMS_TO_TICKS(100)) == pdTRUE) {
            uint8_t len = A7169_GetData(buf, RF_NORMAL_FRAME_LEN - 1);
            if (len > 0 && A7169_ParseNormalData(buf, len, &data)) {
                printf("[433] ID=%02X%02X%08" PRIX32 " PRESS=%.1fkPa TEMP=%dC ACC=%.1fg VENDOR=0x%02X TYPE=0x%02X\n",
                       data.vendor_type, data.sensor_type, data.sensor_id,
                       data.pressure_kpa, data.temperature_c, data.acceleration_g,
                       data.vendor_type, data.sensor_type);
            }
        } else if (GIO1S == 0) {
            A7169_RxFifoReset();
        }
    }
}

static void gpio_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << A7169_GIO1_IRQ_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&io_conf);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(A7169_GIO1_IRQ_PIN, gpio10_isr_handler, NULL);
}

void app_main(void)
{
    if (InitRF() != 0) {
        ESP_LOGE(TAG, "433 RF 初始化失败，请检查接线 (CS:11, CLK:8, DIO:9, GIO1:10)");
        return;
    }
    ESP_LOGI(TAG, "433 RF 初始化成功");

    gpio_init();
    xTaskCreate(rf_recv_task, "rf_recv", 4096, NULL, 9, &s_rf_task);
}
