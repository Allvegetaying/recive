#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "rf_service.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "Application started");

    // 初始化 433 射频接收服务
    if (rf_service_init() == ESP_OK)
    {
        ESP_LOGI(TAG, "433 RF Service running...");
    }
    else
    {
        ESP_LOGE(TAG, "433 RF Service failed to initialize");
    }

    // 主任务保活循环
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
