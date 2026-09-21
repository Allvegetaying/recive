#include "rf_service.h"
#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "A7169.h"

static const char *TAG = "rf_service";

// A7169 GIO1 中断引脚定义 (GPIO10)
#define A7169_GIO1_IRQ_PIN  GPIO_NUM_10

static TaskHandle_t s_rf_recv_task_handle = NULL;
static rf_data_callback_t s_rf_callback = NULL;
static volatile bool s_rf_enabled = true;

// GPIO10 中断服务程序 (A7169 GIO1 下降沿触发)
static void IRAM_ATTR gpio10_isr_handler(void *arg)
{
    if (s_rf_recv_task_handle != NULL)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xTaskNotifyFromISR(s_rf_recv_task_handle, 1, eSetValueWithOverwrite, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

// 433 RF 数据接收任务 (中断响应模式)
static void rf_recv_task(void *pvParameters)
{
    uint8_t rf_buf[64];
    uint32_t notify_value;
    bool rf_enabled_prev = false;

    ESP_LOGI(TAG, "433 RF 接收任务已启动 (中断模式)");

    while (1)
    {
        if (!s_rf_enabled)
        {
            // 未使能状态：天线仍在收帧，A7169 收到帧会把 GIO1 拉低并一直锁存。
            // 若不清走，下次使能后 GIO1 无法产生下降沿，中断接收会失效，故静默排空复位。
            if (GIO1S == 0)
            {
                A7169_RxFifoReset();
            }

            // 丢弃 RF 关闭期间残留的中断通知，避免使能后读到陈旧数据
            xTaskNotifyWait(0, UINT32_MAX, &notify_value, 0);
            rf_enabled_prev = false;
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        // 由关到开的瞬间：先排空残留帧、复位 GIO1，保证后续真帧能触发下降沿中断
        if (!rf_enabled_prev)
        {
            A7169_RxFifoReset();
            ESP_LOGI(TAG, "433 RF 已使能，接收通路已就绪，等待数据中断...");
        }
        rf_enabled_prev = true;

        // 等待 GPIO10 下降沿中断通知 (超时 100ms)
        if (xTaskNotifyWait(0, 0, &notify_value, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            uint8_t len = A7169_GetData(rf_buf, RF_NORMAL_FRAME_LEN - 1);
            if (len > 0)
            {
                rf_normal_data_t rf_data;
                if (A7169_ParseNormalData(rf_buf, len, &rf_data))
                {
                    ESP_LOGI(TAG, "433 normal data: ID=%02X%02X%08" PRIX32
                                  " ACC=%.1fg TEMP=%dC PRESS=%.1fkPa "
                                  "vendor=0x%02X type=0x%02X status=0x%02X",
                                  rf_data.vendor_type,
                                  rf_data.sensor_type,
                                  rf_data.sensor_id,
                                  rf_data.acceleration_g,
                                  rf_data.temperature_c,
                                  rf_data.pressure_kpa,
                                  rf_data.vendor_type,
                                  rf_data.sensor_type,
                                  rf_data.status);

                    if (s_rf_callback != NULL)
                    {
                        s_rf_callback(&rf_data);
                    }
                }
            }
        }
        else
        {
            // 防卡死排空机制：若 GIO1S 持续拉低但未产生中断，及时重置 FIFO
            if (GIO1S == 0)
            {
                A7169_RxFifoReset();
            }
        }
    }
}

esp_err_t rf_service_init(void)
{
    // 1. 初始化 A7169 射频芯片硬件
    if (InitRF() == 0)
    {
        ESP_LOGI(TAG, "433 RF (A7169) 硬件初始化成功");
    }
    else
    {
        ESP_LOGE(TAG, "433 RF (A7169) 硬件初始化失败");
        return ESP_FAIL;
    }

    // 2. 配置 GPIO10 为下降沿中断输入
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << A7169_GIO1_IRQ_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_NEGEDGE
    };
    gpio_config(&io_conf);

    esp_err_t err = gpio_install_isr_service(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGW(TAG, "gpio_install_isr_service returned: %s", esp_err_to_name(err));
    }
    gpio_isr_handler_add(A7169_GIO1_IRQ_PIN, gpio10_isr_handler, (void *)A7169_GIO1_IRQ_PIN);
    ESP_LOGI(TAG, "GPIO10 (A7169 GIO1) 中断服务配置完成");

    // 3. 默认开启接收
    s_rf_enabled = true;

    // 4. 创建接收任务
    BaseType_t ret = xTaskCreate(rf_recv_task, "rf_recv", 4096, NULL, 9, &s_rf_recv_task_handle);
    if (ret != pdPASS)
    {
        ESP_LOGE(TAG, "创建 rf_recv 任务失败");
        return ESP_FAIL;
    }

    return ESP_OK;
}

void rf_service_register_callback(rf_data_callback_t cb)
{
    s_rf_callback = cb;
}

void rf_service_set_enable(bool enable)
{
    s_rf_enabled = enable;
}

bool rf_service_is_enabled(void)
{
    return s_rf_enabled;
}
