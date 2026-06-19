/**
 * @file btcfg_general.c
 * @brief File chua cac dinh nghia lien quan den macros, enum, struct,...dung chung khong lien quan gi den xu ly GAP/GATT
 * 
 * @author Luong Huu Phuc
 * @date 2026/03/03
 */

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#include "btcfg_general.h"
#include "esp_log.h"

/* Khoi tao toan cuc voi gia tri ban dau la NULL tai file nay de khong anh huong den cac file xu ly GAP/GATT */
SemaphoreHandle_t g_btcfg_mutex_handle = NULL; 

#ifdef __cplusplus
}
#endif //__cplusplus
