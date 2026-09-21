#ifndef RF_SERVICE_H
#define RF_SERVICE_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "A7169.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 433 射频数据接收回调函数类型
 */
typedef void (*rf_data_callback_t)(const rf_normal_data_t *data);

/**
 * @brief 初始化 433 射频服务 (包含 A7169 初始化、中断配置及接收任务创建)
 * 
 * @return esp_err_t ESP_OK 表示初始化成功
 */
esp_err_t rf_service_init(void);

/**
 * @brief 注册 433 数据接收回调函数 (可选)
 * 
 * @param cb 回调函数指针，收到并解析有效数据时调用
 */
void rf_service_register_callback(rf_data_callback_t cb);

/**
 * @brief 使能/暂停 433 接收
 * 
 * @param enable true 为使能，false 为暂停
 */
void rf_service_set_enable(bool enable);

/**
 * @brief 查询当前 433 接收使能状态
 * 
 * @return true 已使能
 * @return false 已暂停
 */
bool rf_service_is_enabled(void);

#ifdef __cplusplus
}
#endif

#endif /* RF_SERVICE_H */
