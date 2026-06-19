/**
 * @brief btcfg_central_gap.c
 * @brief Dinh nghia cac API xu ly GAP cho BLE Central
 * 
 * @author Luong Huu Phuc
 * @date 2026/03/03
 */

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "btcfg_central_proc.h"

#include <string.h>

#include "host/ble_gap.h"
#include "host/ble_hs.h"
#include "host/util/util.h"

static char g_target_name[32] = {0}; // Mang chuoi luu ten thiet bi muc tieu muon ket noi den

/* ============================ HELPER FUNCTION ============================ */

/* Ham thuc hien so sanh chuoi ten thiet bi nhan duoc va ten thiet bi muc tieu muon ket noi */
static bool adv_name_match(const struct ble_hs_adv_fields *fields){
  if(!fields->name) return false;

  if(strlen(g_target_name) == 0) return false;
  if(g_target_name[0] == '\0') return false;

  size_t target_len = strlen(g_target_name);
  if(fields->name_len != target_len) return false;

  // So sanh dung strncmp an toan hon vi gioi han so ky tu (n) giup tranh truy cap bo nho ngoai pham vi
  // if(strncmp((const char*)fields->name, g_target_name, fields->name_len) == 0) return true;

  return (memcmp(fields->name, g_target_name, target_len) == 0);
}

/* ============================ PUBLIC GAP API ============================ */

btcfg_err_t btcfg_central_start_scan(uint32_t duration_ms){
  if(!g_btcfg_central_ctx.is_initialized) return BTCFG_ERR_NOT_READY;

  // Neu dang quet roi thi thoat luon de tranh ghi de cau hinh Host
  if(g_btcfg_central_ctx.is_scanning) return BTCFG_OK;

  // Cau hinh tham so quet song chu dong (Active Scan) de ep Slave nha tron goi Scan Response
  struct ble_gap_disc_params params = {0};
  params.passive = 0;
  params.itvl = 0x0060;    // iteration interval (quang lap lai scan trong 0.625ms) = 96 * 0.625 = 60ms
  params.window = 0x0030;  // Window: 48 * 0.625 = 30ms (Duty Cycle 50% tiet kiem tai nguyen) 

  // Truyen truc tiep duration_ms chuan don vi ms theo dung tai lieu NimBLE
  int rc = ble_gap_disc(BLE_OWN_ADDR_PUBLIC, duration_ms, &params, btcfg_central_gap_event, NULL);
  if(rc != 0){
    BTCFG_LOGE("[CENTRAL] ble_gap_disc failed: %d", rc);
    return BTCFG_ERR_FAIL;
  }

  g_btcfg_central_ctx.state = BTCFG_STATE_SCANNING;
  g_btcfg_central_ctx.is_scanning = true;
  BTCFG_LOGI("[CENTRAL] Scanning stared...");

  return BTCFG_OK;
}

/*-----------------------------------------------------------*/

btcfg_err_t btcfg_central_stop_scan(void){
  if(!g_btcfg_central_ctx.is_initialized) return BTCFG_ERR_NOT_READY;

  // Neu da dung scan roi thi thoat luon de tranh ghi de cau hinh Host
  if(!g_btcfg_central_ctx.is_scanning) return BTCFG_OK;

  int rc = ble_gap_disc_cancel();
  if(rc != 0) return BTCFG_ERR_FAIL;

  g_btcfg_central_ctx.is_scanning = false;
  return BTCFG_OK;
}

/*-----------------------------------------------------------*/

btcfg_err_t btcfg_central_connect(const char *name){
  if(name == NULL) return BTCFG_ERR_INVALID_ARG;
  if(!g_btcfg_central_ctx.is_initialized) return BTCFG_ERR_NOT_READY;

  // Neu dang ket noi roi thi tu choi phat lenh quet de
  if(g_btcfg_central_ctx.state == BTCFG_STATE_CONNECTED) return BTCFG_OK;

  // Copy ten thiet bi Peripheral input vao vung dem toan cuc g_target_name bao ve bien an toan
  strncpy(g_target_name, name, sizeof(g_target_name) - 1);
  g_target_name[sizeof(g_target_name) - 1] = '\0';

  return btcfg_central_start_scan(SCAN_DURATION_MS);
}

/*-----------------------------------------------------------*/

btcfg_err_t btcfg_central_disconnect(void){
  if(g_btcfg_central_ctx.state != BTCFG_STATE_CONNECTED || g_btcfg_central_ctx.conn_handle == BLE_HS_CONN_HANDLE_NONE){
    return BTCFG_ERR_NO_CONN;
  }

  // Phat lenh chu dong huy lien ket link vat ly tu phia Central Master
  int rc = ble_gap_terminate(g_btcfg_central_ctx.conn_handle, BLE_ERR_REM_USER_CONN_TERM);
  return (rc == 0) ? BTCFG_OK : BTCFG_ERR_FAIL;
}

/* ============================ INTERNAL GAP EVENT HANDLER FUNCTION ============================ */

int btcfg_central_gap_event(struct ble_gap_event *event, void *arg){
  (void)arg;

  switch(event->type){

    /* 1. Su kien ket thuc tien trinh ket noi */
    case BLE_GAP_EVENT_CONNECT:{
      if(event->connect.status == 0){
        g_btcfg_central_ctx.conn_handle = event->connect.conn_handle;
        g_btcfg_central_ctx.state = BTCFG_STATE_CONNECTED;
        g_btcfg_central_ctx.is_scanning = false; // Tat luon co quet khi da co ket noi
        g_btcfg_central_ctx.srv_notify_enabled = false; // Chờ thiet bi bat CCCD o su kien Subscribe
        
        BTCFG_LOGI("[CENTRAL] Connected successfully ! Conn_handle=%u", g_btcfg_central_ctx.conn_handle);

        /* Ngay sau khi vua connected, ESP32 chu dong gui lenh Request dam phan MTU */
        int rc = ble_gattc_exchange_mtu(g_btcfg_central_ctx.conn_handle, NULL, NULL);
        if (rc != 0) BTCFG_LOGI("MTU exchange request failed !");

        if(g_central_connect_cb != NULL) g_central_connect_cb(g_btcfg_central_ctx.conn_handle);
        
      }
      else{
        g_btcfg_central_ctx.is_scanning = false;
        g_btcfg_central_ctx.state = BTCFG_STATE_IDLE;  
        g_btcfg_central_ctx.conn_handle = BLE_HS_CONN_HANDLE_NONE; 
        BTCFG_LOGE("[CENTRAL] Connection failed, status=%d", event->connect.status);
      }
      return 0;
    }

    /* 2. Su kien bi ngat ket noi BLE */
    case BLE_GAP_EVENT_DISCONNECT:{
      g_btcfg_central_ctx.conn_handle = BLE_HS_CONN_HANDLE_NONE;
      g_btcfg_central_ctx.state = BTCFG_STATE_DISCONNECTED;
      g_btcfg_central_ctx.srv_notify_enabled = false;
      g_btcfg_central_ctx.is_scanning = false;

      BTCFG_LOGW("[CENTRAL] Peripheral disconnected, reason=%d", event->disconnect.reason);

      if(g_central_disconnect_cb != NULL) g_central_disconnect_cb();
      return 0;
    }

    /* 3. Su kien bat duoc goi tin quang ba (Discovery Device) tu cac thiet bi xung quanh */
    case BLE_GAP_EVENT_DISC:{
      struct ble_hs_adv_fields fields; // Truong Host Advertising 
      int rc = ble_hs_adv_parse_fields(&fields, event->disc.data, event->disc.length_data);
      if(rc != 0) return 0;

      // Neu quet trung ten thiet bi muc tieu muon ket noi
      if(adv_name_match(&fields)){
        BTCFG_LOGI("[Central] Target device found. Stopping scan and connecting...");

        // Ep huy tien trinh scan truoc khi phat lenh thiet lap ket noi de bao ve tai nguyen Host
        ble_gap_disc_cancel();
        g_btcfg_central_ctx.is_scanning = false;
        g_btcfg_central_ctx.state = BTCFG_STATE_CONNECTING;

        // Tien hanh go cua ket noi toi dia chi MAC vat ly cua Peripheral muc tieu
        rc = ble_gap_connect(g_btcfg_central_ctx.own_addr_type, &event->disc.addr, 30000, NULL, btcfg_central_gap_event, NULL);
        if(rc != 0){
          BTCFG_LOGE("[Central] ble_gap_conntect failed, rc=%d", rc);
          g_btcfg_central_ctx.state = BTCFG_STATE_IDLE;
        }
      }
      return 0;
    }

    /**
     * 4. Đón su kien thiet bi doi tac bat/tat flag subscribe notify chu dong (CCCD) tren GATT Server cua Central tai GAP
     * Hanh dong thuc hien bat/tat tai GATT nhung su kien nhan tai GAP
     * (ESP32 Central lam GATT Server)
     */
    case BLE_GAP_EVENT_SUBSCRIBE:{
      // Neu su kien no ra tu ket noi hien tai va GATT Client co su thay doi trang thai dang ky Notify
      if(event->subscribe.conn_handle == g_btcfg_central_ctx.conn_handle){
        g_btcfg_central_ctx.srv_notify_enabled = event->subscribe.cur_notify;
        BTCFG_LOGI("[CENTRAL] Client sub changed. Notify:%s", g_btcfg_central_ctx.srv_notify_enabled ? "enabled" : "disabled");
      }
      return 0;
    }

    /**
     * 5. Đón su kien nhan luong du lieu Notify chu dong tu GATT Server tai GAP de chuan bi dua vao xu ly trong GATT
     * Hanh dong notify duoc thuc hien tai GATT nhung su kien nhan va xu ly ngay tai GAP
     * (ESP32 Central lam GATT Client)
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
        if(rc == 0 && g_central_clt_ntf_evt_cb != NULL){
          g_central_clt_ntf_evt_cb(event->notify_rx.conn_handle, event->notify_rx.attr_handle, buf, len);
        }
      }
      return 0;
    }

    case BLE_GAP_EVENT_MTU:{
      BTCFG_LOGI("[CENTRAL] MTU updated to %d bytes", event->mtu.value)
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

    default:
      return 0;
  }    
}

#ifdef __cplusplus
}
#endif // __cplusplus