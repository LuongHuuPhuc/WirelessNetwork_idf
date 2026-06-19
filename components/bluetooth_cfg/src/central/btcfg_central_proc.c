/**
 * @brief btcfg_central_proc.c
 * @brief Trien khai API chung cho BLE GAP Central (Master) voi GATT la Server/Client Dual-Role
 * 
 * @author Luong Huu Phuc
 * @date 2026/03/03
 */

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "btcfg_central_proc.h"

#include <string.h>

#include "nvs_flash.h"
#include "esp_err.h"
#include "esp_nimble_hci.h"

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"

#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

/* ============================ GLOBAL ============================ */

/* Khoi tao Struct quan ly trong thai (Context Machine) cuc bo cho Central */
btcfg_central_context_t g_btcfg_central_ctx = {
  .conn_handle = BLE_HS_CONN_HANDLE_NONE,
  .state = BTCFG_STATE_IDLE,
  .is_initialized = false,
  .is_scanning = false,
  .srv_notify_enabled = false,
  .tx_attr_handle = 0,
  .own_addr_type = BLE_OWN_ADDR_PUBLIC // Dia chi MAC public
};

btcfg_central_connect_cb_t g_central_connect_cb = NULL;
btcfg_central_disconnect_cb_t g_central_disconnect_cb = NULL;

// Buffer chua dinh danh thiet bi cua Central vai tro GATT Server
static char g_central_device_name[32] = "ESP32_CENTRAL_PHUC";

/* Lien ket extern con tro toi struc ble_gatt_svc_def (ham nay duoc dinh nghia trong file `btcfg_central_gatt.c`) */
extern const struct ble_gatt_svc_def *btcfg_central_gatt_svcs_get(void);

/* ============================ NIMBLE HOST TASK ============================ */

/* Callback de chay FreeRTOS task rieng biet de duy tri luong xu ly Bluetooth Host (chay luong doc lap voi chuong trinh chinh) */
static void btcfg_central_host_task(void *pvParameters){
  (void)pvParameters;
  nimble_port_run();

  BTCFG_LOGW("[CENTRAL] NimBLE Host Task exited ! Deleting task...");
  nimble_port_freertos_deinit(); // nimble_disable (chinh la vTaskDelete(NULL))
}

/*-----------------------------------------------------------*/

/* Callback duoc Host stack tu dong kich no ngay khi qua trinh dong bo giua BLE controller phan cung va BLE Host phan mem hoan tat */
static void btcfg_central_on_sync(void){
  int rc = ble_hs_id_infer_auto(0, &g_btcfg_central_ctx.own_addr_type);
  if(rc != 0){
    BTCFG_LOGE("[CENTRAL] BLE_host sync failed: %d", rc);
    return;
  }
  BTCFG_LOGI("[CENTRAL] BLE_host synced");

  // Sau khi dong bo xong, lap tuc phat song quet cac thiet bi xung quanh
  btcfg_err_t ret = btcfg_central_start_scan(SCAN_DURATION_MS);
  if(ret != BTCFG_OK){
    BTCFG_LOGE("[CENTRAL] Start scanning failed: %d", ret);
  } 
}

/* ============================ PUBLIC API ============================ */

btcfg_err_t btcfg_central_init(void){
  if(g_btcfg_central_ctx.is_initialized) return BTCFG_ERR_ALREADY_INIT;

  /* Khoi tao phan vung nho NVS Flash */
  esp_err_t ret = nvs_flash_init();
  if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
    ret = nvs_flash_erase();
    if(ret != ESP_OK) return BTCFG_ERR_FAIL;
    ret = nvs_flash_init();
  }
  if(ret != ESP_OK){
    BTCFG_LOGE("[CENTRAL] nvs_flash_init failed: %d", ret);
    return BTCFG_ERR_FAIL;
  }

  /* Khoi tao lop ket noi VHCI (Virtual Host-Controller interface) giua BLE Host va Controller */
  // ret = esp_nimble_hci_init();
  // if(ret != ESP_OK){
  //   BTCFG_LOGE("[CENTRAL] esp_nimble_hci_init failed: %d", ret);
  //   return BTCFG_ERR_FAIL;
  // }

  /* Khoi tao Controller va NimBLE host (khong can dung esp_nimble_hci_init() nua) */
  ret = nimble_port_init();
  if(ret != ESP_OK){
    BTCFG_LOGE("[CENTRAL] nimble_port_init failed: %d", ret);
  }

  /* Khoi tao Mutex tai Central */
  if(g_btcfg_mutex_handle == NULL){
    g_btcfg_mutex_handle = xSemaphoreCreateMutex();
    if(g_btcfg_mutex_handle == NULL){
      BTCFG_LOGE("[CENTRAL] Create Mutex failed");
      return BTCFG_ERR_FAIL;
    }
  }

  // Dang ky con tro ham dong bo cua Host Central
  ble_hs_cfg.sync_cb = btcfg_central_on_sync;

  /* Khoi tao va nap cac Service BLE tieu chuan he thong */
  ble_svc_gap_init();  // Dky GAP Service chuan cua BLE Server
  ble_svc_gatt_init(); // Dky GATT Service chuan 

  /* Thiet lap ten thiet bi ma cac phan khac cua Stack co the dung */
  int rc = ble_svc_gap_device_name_set(g_central_device_name);
  if(rc != 0){
    BTCFG_LOGE("[CENTRAL] Set device name failed: %d", rc);
    return BTCFG_ERR_FAIL;
  }

  // Dem so luong thuoc tinh va nap mang dich vu GATT Table noi bo vao database
  rc = ble_gatts_count_cfg(btcfg_central_gatt_svcs_get());
  if(rc != 0) return BTCFG_ERR_FAIL;

  rc = ble_gatts_add_svcs(btcfg_central_gatt_svcs_get());
  if(rc != 0) return BTCFG_ERR_FAIL;

  /* Set trang thai truoc khi host task chay */
  g_btcfg_central_ctx.is_initialized = true;
  g_btcfg_central_ctx.state = BTCFG_STATE_INITIALIZED;

  nimble_port_freertos_init(btcfg_central_host_task); // Callback function *TaskFucntion_t(void*) btcfg_host_task

  BTCFG_LOGI("[CENTRAL] BLE stack Initialized OK !");
  return BTCFG_OK;
}

/*-----------------------------------------------------------*/

bool btcfg_central_is_connected(void){
  return (g_btcfg_central_ctx.conn_handle != BLE_HS_CONN_HANDLE_NONE);
}

/*-----------------------------------------------------------*/

uint16_t btcfg_central_get_conn_handle(void){
  return (g_btcfg_central_ctx.conn_handle);
}

/* ============================ CALLBACK FUCNTION REGISTER ============================ */

void btcfg_central_register_connect_callback(btcfg_central_connect_cb_t cb){
  g_central_connect_cb = cb;
}

/*-----------------------------------------------------------*/

void btcfg_central_register_disconnect_callback(btcfg_central_disconnect_cb_t cb){
  g_central_disconnect_cb = cb;
}

#ifdef __cplusplus
}
#endif // __cplusplus