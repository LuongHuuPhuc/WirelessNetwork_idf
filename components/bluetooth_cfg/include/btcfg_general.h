/**
 * @file btcfg_general.h
 * @brief File chua cac khai bao lien quan den macros, enum, struct,...dung chung khong lien quan gi den xu ly GAP/GATT
 * 
 * @author Luong Huu Phuc
 * @date 2026/03/03
 */

#ifndef BTCFG_GENERAL_INCLUDE_H
#define BTCFG_GENERAL_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#pragma once

#include "stdint.h"
#include "stdbool.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

/* Lua chon phuong thuc BLE GAP muon su dung */
#define BTCFG_PERIPHERAL_USING  1     /* GAP Peripheral */
// #define BTCFG_CENTRAL_USING     1     /* GAP Central */

#define BTCFG_TAG  "BTCFG"
#define BTCFG_LOGI(...) ESP_LOGI(BTCFG_TAG, __VA_ARGS__);
#define BTCFG_LOGW(...) ESP_LOGI(BTCFG_TAG, __VA_ARGS__);
#define BTCFG_LOGE(...) ESP_LOGE(BTCFG_TAG, __VA_ARGS__);

/* Them Semaphore/Mutex de tranh thread bi chong nhau */
#define BTCFG_LOCK(_mutex_handle_)    xSemaphoreTake((_mutex_handle_), portMAX_DELAY)
#define BTCFG_UNLOCK(_mutex_handle_)  xSemaphoreGive((_mutex_handle_))

extern SemaphoreHandle_t g_btcfg_mutex_handle; 

/* ============================ ERROR TYPE ============================ */

/* Ma loi chung tra ve khi muon debug */
typedef enum {
  BTCFG_OK = 0,                 /* !< Toan tu/bien hop le/thanh cong */
  BTCFG_ERR_INVALID_ARG   = -1, /* !< Gia tri khong hop le */
  BTCFG_ERR_NOT_READY     = -2, /* !< BLE stack chua khoi tao/chua san sang */
  BTCFG_ERR_ALREADY_INIT  = -3, /* !< BLE stack da khoi tao */
  BTCFG_ERR_NO_CONN       = -4, /* !< Khong co BLE duoc ket noi */
  BTCFG_ERR_NO_CHAR       = -5, /* !< Khong tim thay Characteristic */
  BTCFG_ERR_FAIL          = -6  /* !< Loi chung */
} btcfg_err_t;

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // BTCFG_GENERAL_INCLUDE_H