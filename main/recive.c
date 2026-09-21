#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "rf_service.h"

static const char *TAG = "main";
static uint32_t s_rx_count = 0;

static void on_433_data_received(const rf_normal_data_t *data)
{
    s_rx_count++;
    printf("\n----------------------------------------\n");
    printf("[433 RF] Received Packet #%" PRIu32 "\n", s_rx_count);
    printf("  ID     : %02X%02X%08" PRIX32 "\n", data->vendor_type, data->sensor_type, data->sensor_id);
    printf("  PRESS  : %.1f kPa\n", data->pressure_kpa);
    printf("  TEMP   : %d C\n", data->temperature_c);
    printf("  ACC    : %.1f g\n", data->acceleration_g);
    printf("  VENDOR : 0x%02X\n", data->vendor_type);
    printf("  TYPE   : 0x%02X\n", data->sensor_type);
    printf("  STATUS : 0x%02X\n", data->status);
    printf("----------------------------------------\n\n");
}

void app_main(void)
{
    ESP_LOGI(TAG, "Application started");
    ESP_LOGI(TAG, "A7169 Pins: SCS=GPIO11, SCK=GPIO8, SDIO=GPIO9, GIO1=GPIO10");

    rf_service_register_callback(on_433_data_received);

    if (rf_service_init() == ESP_OK)
    {
        ESP_LOGI(TAG, "433 RF Service running...");
    }
    else
    {
        ESP_LOGE(TAG, "433 RF Service failed to initialize");
    }

    uint32_t sec = 0;
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(5000));
        sec += 5;
        ESP_LOGI(TAG, "[%" PRIu32 " s] 433 listening... total packets: %" PRIu32, sec, s_rx_count);
    }
}

