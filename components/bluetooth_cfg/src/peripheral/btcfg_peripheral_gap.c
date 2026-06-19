/**
 * @brief btcfg_peripheral_gap.c
 * @brief Dinh nghia cac API xu ly GAP cho BLE Peripheral
 * 
 * @author Luong Huu Phuc
 * @date 2026/03/03
 */

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "btcfg_peripheral_proc.h"

#include <string.h>

#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"

/* ============================ PUBLIC GAP API ============================ */

btcfg_err_t btcfg_peripheral_start_advertising(void){
  if(!g_btcfg_peripheral_ctx.is_initialized) return BTCFG_ERR_NOT_READY;

  // Neu dang quang ba roi thi thoat luon de tranh ghi de cau hinh Host
  if(g_btcfg_peripheral_ctx.is_advertising) return BTCFG_OK;

  struct ble_hs_adv_fields fields;
  struct ble_gap_adv_params adv_params;

  memset(&fields, 0, sizeof(fields));
  memset(&adv_params, 0, sizeof(adv_params));

  // Cau hinh co quang ba chuan BLE: Discoverable General + BR/EDR Unsupport (Bluetooth Classic) 
  fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

  const char *name = ble_svc_gap_device_name();
  fields.name = (const uint8_t *)name;
  fields.name_len = strlen(name);
  fields.name_is_complete = 1;

  int rc = ble_gap_adv_set_fields(&fields);
  if(rc != 0) return BTCFG_ERR_FAIL;

  // Cau hinh che do ket noi cong khai khong dinh huong (Undirected Connectable)
  adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
  adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

  // Dang ky ham btcfg_peripheral_gap_event lam core xu ly vong lap xu kien khi quang ba
  rc = ble_gap_adv_start(g_btcfg_peripheral_ctx.own_addr_type, NULL, BLE_HS_FOREVER, &adv_params, btcfg_peripheral_gap_event, NULL);
  if(rc != 0) return BTCFG_ERR_FAIL;

  g_btcfg_peripheral_ctx.state = BTCFG_STATE_ADVERTISING;
  g_btcfg_peripheral_ctx.is_advertising = true;
  BTCFG_LOGI("[PERIPHERAL] Advertising...");

  return BTCFG_OK;
}

/*-----------------------------------------------------------*/

btcfg_err_t btcfg_peripheral_stop_advertising(void){
  if(!g_btcfg_peripheral_ctx.is_initialized) return BTCFG_ERR_NOT_READY;

  int rc = ble_gap_adv_stop();
  if(rc != 0) return BTCFG_ERR_FAIL;

  g_btcfg_peripheral_ctx.state = BTCFG_STATE_ADV_STOPPED;
  g_btcfg_peripheral_ctx.is_advertising = false;
  BTCFG_LOGI("[PERIPHERAL] Advertising stopped.");

  return BTCFG_OK;
}

/* ============================ INTERNAL GAP EVENT HANDLER FUNCTION ============================ */

int btcfg_peripheral_gap_event(struct ble_gap_event *event, void *arg){
  (void)arg;

  switch(event->type){

    /* 1. Su kien co thiet bi Central ket noi vao ESP32 */
    case BLE_GAP_EVENT_CONNECT:{
      if(event->connect.status == 0){
        g_btcfg_peripheral_ctx.conn_handle = event->connect.conn_handle;
        g_btcfg_peripheral_ctx.state = BTCFG_STATE_CONNECTED;
        g_btcfg_peripheral_ctx.is_advertising = false;
        g_btcfg_peripheral_ctx.srv_notify_enabled = false;  // Chờ thiet bi bat CCCD o su kien Subscribe

        // BTCFG_LOGI("[PERIPHERAL] Central connected successfully, handle=%d", g_btcfg_peripheral_ctx.conn_handle);

        /* Ngay sau khi vua connected, ESP32 chu dong gui lenh Request dam phan MTU len Central theo MTU da set trong menuconfig */
        int rc = ble_gattc_exchange_mtu(g_btcfg_peripheral_ctx.conn_handle, NULL, NULL);
        if (rc != 0){
          /* Phan loai ma loi: Neu ma tra ve la 0x12 BLE_HS_EALREADY nghia la phia Central da lam xong truoc roi */
          if(rc != BLE_HS_EALREADY) BTCFG_LOGE("[PERIPHERAL] MTU exchange request failed !");
        }
        else BTCFG_LOGI("[PERIPHERAL] MTU exchange OK !");

        // Kich hoat callback chuyen tiep du lieu len Application Layer xu ly
        if(g_peripheral_connect_cb != NULL) g_peripheral_connect_cb(g_btcfg_peripheral_ctx.conn_handle);
      }
      else{
        // Danh dau trang thai loi de Host tu thoat, khong goi ham API truc tiep tai day
        g_btcfg_peripheral_ctx.is_advertising = false;
        g_btcfg_peripheral_ctx.state = BTCFG_STATE_DISCONNECTED;
        g_btcfg_peripheral_ctx.conn_handle = BLE_HS_CONN_HANDLE_NONE; 
        // BTCFG_LOGE("[PERIPHERAL] Connection failed, status=%d", event->connect.status);
      }
      return 0;
    }

    /* 2. Su kien bi ngat ket noi BLE */
    case BLE_GAP_EVENT_DISCONNECT:{
      g_btcfg_peripheral_ctx.conn_handle = BLE_HS_CONN_HANDLE_NONE;
      g_btcfg_peripheral_ctx.state = BTCFG_STATE_DISCONNECTED;
      g_btcfg_peripheral_ctx.srv_notify_enabled = false;
      g_btcfg_peripheral_ctx.is_advertising = false;

      // BTCFG_LOGW("[PERIPHERAL] Central disconnected, reason=%d", event->disconnect.reason);

      // Kich hoat callback chuyen tiep du lieu len Application Layer xu ly
      if(g_peripheral_disconnect_cb != NULL) g_peripheral_disconnect_cb();

      // Khi mat ket noi, khong truc tiep goi Advertising tai ham Ngat Event nay gay sap IDLE0
      return 0;
    }

    /**
     * 3. Đón su kien thiet bi doi tac bat-tat flag subscribe notify chu dong (CCCD) tren GATT Server cua Peripheral tai GAP
     * Hanh dong thuc hien bat/tat tai GATT nhung su kien nhan tai GAP
     * (ESP32 Peripheral lam GATT Server)
     */
    case BLE_GAP_EVENT_SUBSCRIBE:{
      // Neu su kien no ra tu ket noi hien tai va GATT Client co su thay doi trang thai dang ky Notify
      if(event->subscribe.conn_handle == g_btcfg_peripheral_ctx.conn_handle){
        g_btcfg_peripheral_ctx.srv_notify_enabled = event->subscribe.cur_notify;
        BTCFG_LOGI("[PERIPHERAL] Client sub changed. Notify:%s", g_btcfg_peripheral_ctx.srv_notify_enabled ? "enabled" : "disabled");
      }
      return 0;
    }

    /**
     * 4. Đón su kien nhan luong du lieu Notify chu dong tu GATT Server tai GAP de chuan bi dua vao xu ly trong GATT
     * Hanh dong notify duoc thuc hien tai GATT nhung su kien nhan va xu ly ngay tai GAP
     * (ESP32 Peripheral lam GATT Client)
     */
    case BLE_GAP_EVENT_NOTIFY_RX:{
      // Neu su kien nhan thong bao chuan (Notification), khong phai Indication
      if(event->notify_rx.indication == 0){
        uint16_t len = OS_MBUF_PKTLEN(event->notify_rx.om);
        uint8_t buf[LOCAL_SERVER_BUF_SIZE];

        if(len >= sizeof(buf)) len = sizeof(buf) - 1;

        // Trich xuat du lieu nhi phan sạch tu bo nho dem mbuf ngam cua ngat
        int rc = os_mbuf_copydata(event->notify_rx.om, 0, len, buf);

        // Day thang khoi du lieu qua con tro ham callback Client len tang ung dung giai ma
        if(rc == 0 && g_peripheral_clt_ntf_evt_cb != NULL){
          g_peripheral_clt_ntf_evt_cb(event->notify_rx.conn_handle, event->notify_rx.attr_handle, buf, len);
        }
      }
      return 0;
    }

    /* Don nhan su kien ket thuc dam phan MTU giua 2 thiet bi */
    case BLE_GAP_EVENT_MTU:{
      BTCFG_LOGI("[PERIPHERAL] MTU updated to %d bytes", event->mtu.value)
      return 0;
    }

    case BLE_GAP_EVENT_CONN_UPDATE:{
      return 0;
    }

    case BLE_GAP_EVENT_ADV_COMPLETE:{
      /* ... */
      return 0;
    }

    case BLE_GAP_EVENT_NOTIFY_TX:{
      /* ... */
      return 0;
    }

    default:{
      return 0;
    }
  }
}

#ifdef __cplusplus
}
#endif // __cplusplus