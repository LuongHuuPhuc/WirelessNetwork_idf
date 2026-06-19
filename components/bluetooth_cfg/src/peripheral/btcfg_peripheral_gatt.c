/**
 * @brief btcfg_peripheral_gatt.c
 * @brief Dinh nghia cac API Xu ly GATT(Server/Client) cho BLE GAP Peripheral
 * 
 * @author Luong Huu Phuc
 * @date 2026/03/03
 */

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "btcfg_peripheral_proc.h"

#include <string.h>

#include "host/ble_gatt.h"
#include "os/os_mbuf.h"

/* ============================ GLOBAL ============================ */

btcfg_peripheral_srv_read_cb_t g_peripheral_srv_read_cb = NULL;           /* Peripheral GATT Server xu ly Read-Request tu GATT Client */
btcfg_peripheral_srv_write_cb_t g_peripheral_srv_write_cb = NULL;         /* Peripheral GATT Server xu ly Write-Request tu GATT Client */    
btcfg_peripheral_clt_notify_evt_cb_t g_peripheral_clt_ntf_evt_cb = NULL;  /* Peripheral GATT Client xu ly Notify nhan tu GATT Server */
btcfg_peripheral_clt_read_rsp_cb_t g_peripheral_clt_read_rsp_cb = NULL;   /* Peripheral GATT Client xu ly phan hoi Read-Request tu GATT Server */

__attribute__((unused)) static char g_tx_text[32] = "Hello from ESP32"; /* Demo String */

// Cau truc luu tru du lieu dong cuc bo cho GATT Server (Ho tro du lieu nhi phan)
static uint8_t s_peripheral_srv_local_data[LOCAL_SERVER_BUF_SIZE];
static uint16_t s_peripheral_srv_local_len = 0;

/**
 * @note Luong hoat dong cua UUID
 * Vi du dien thoai connect vao ESP32
 * 1. Dien thoai discover service
 * 2. Thay service co UUID = g_peripheral_svc_uuid
 * 3. Dien thoai discover characteristic ben trong service do 
 * 4. Thay: 
 *  - char g_peripheral_tx_uuid
 *  - char g_peripheral_rx_uuid 
 * 5. Neu muon nhan du lieu tu ESP32 
 *  - subscribe vao g_peripheral_tx_uuid
 * 6. Neu muon ghi du lieu xuong ESP32 
 *  - write vao g_peripheral_rx_uuid
 * 
 * UUID: 
 *  - la danh tinh logic do nguoi dung dinh nghia, on dinh giua cac lan chay neu ban giu nguyen 
 *  - NimBLE luu UUIS 128bit theo little-endian byte order nen code se nhin nguoc so voi chuoi UUID dien thoai hien thi
 * Handle: 
 *  - la so do stack gan khi tao GATT database 
 *  - vi du: 0x0025
 *  - co the thay doi neu cau truc service thay doi
 * Nen trong thiet ke BLE:
 *  - UUID de dinh nghia cai gi la cai gi
 *  - Handle de stack thao tac runtime
 */

/* Cau hinh Custom UUID de thiet bi khac ket noi vao doc/ghi tham so cau hinh nguoc Peripheral */
/* UUID cua Service chinh 6E400001-B5A3-F393-E0A9-E50E24DCCA9E (6E400001 - cho Service) */
static const ble_uuid128_t g_peripheral_svc_uuid = 
  BLE_UUID128_INIT(0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0,
                   0x93, 0xF3, 0xA3, 0xB5, 0x01, 0x00, 0x40, 0x6E);

/* Characteristic de Client write du lieu xuong ESP32 6E400002-B5A3-F393-E0A9-E50E24DCCA9E (6E400002 - cho RX) */
static const ble_uuid128_t g_peripheral_rx_uuid =
  BLE_UUID128_INIT(0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0,
                   0x93, 0xF3, 0xA3, 0xB5, 0x02, 0x00, 0x40, 0x6E);

/* Characteristic de Client read va ESP32 notify 6E400003-B5A3-F393-E0A9-E50E24DCCA9E (6E400003 - cho TX) */
static const ble_uuid128_t g_peripheral_tx_uuid =
  BLE_UUID128_INIT(0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0,
                   0x93, 0xF3, 0xA3, 0xB5, 0x03, 0x00, 0x40, 0x6E);

/* ============================ GATT SERVER INTERNAL CALLBACK HELPER ============================ */

/**
 * @note Callback GATT Server thuc hien dieu phoi luong truy cap du lieu (Read/Write) tu GATT Client ben ngoai
 */
static int btcfg_peripheral_srv_internal_gatt_access_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg){
  (void)arg; 

  /**
   * Chot chan: Neu Client chi thao tac bat/tat Notify CCCD hoac doc Descriptor (_DSC) -> Thoat nhan va bao thanh cong 
   * Tuyet doi khong cho phep boc ctxt->chr->uuid o phia duoi gay hong Stack Canary IDLE0
   */
  if(ctxt->op == BLE_GATT_ACCESS_OP_READ_DSC || ctxt->op == BLE_GATT_ACCESS_OP_WRITE_DSC) return 0;

  /* In ra params cua context */
  BTCFG_LOGI("[PERIPHERAL] [GATT Server Access] op=%d attr=0x%04X conn=%u", ctxt->op, attr_handle, conn_handle);

  // Sau chot chan nay, ctxt->op chac chan la thao tac _CHR, doc UUID an toan 100%
  const ble_uuid_t *uuid = ctxt->chr->uuid;

  /* 1. Xu ly Characteristic phat du lieu TX (Read Request tu Client va Notify) */
  if(ble_uuid_cmp(uuid, &g_peripheral_tx_uuid.u) == 0){

    /* Chi goi read callback khi day thuc su la thao tac READ tu Client (conn_handle = 1, khong phai conn_handle = 0xFFFF) */
    if(ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR){
      if(conn_handle != BLE_HS_CONN_HANDLE_NONE){
        if(g_peripheral_srv_read_cb != NULL) g_peripheral_srv_read_cb(attr_handle, conn_handle);
        
        // Day du lieu nhi phan an toan tu bo nho tam s_peripheral_srv_local_data vao mbuf cua NimBLE
        // BTCFG_LOCK(g_btcfg_mutex_handle);
        int rc = os_mbuf_append(ctxt->om, s_peripheral_srv_local_data, s_peripheral_srv_local_len);
        // BTCFG_UNLOCK(g_btcfg_mutex_handle);

        return (rc == 0) ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
      }
    }
    return BLE_ATT_ERR_UNLIKELY;
  }

  /* 2. Xu ly Characteristic nhan du lieu RX (Write Request tu Client) */
  if(ble_uuid_cmp(uuid, &g_peripheral_rx_uuid.u) == 0){

    /* Chi xu ly write callback khi day la thao tac WRITE tu Client */
    if(ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR){
      uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
      uint8_t buf[LOCAL_SERVER_BUF_SIZE];

      if(len >= sizeof(buf)) len = sizeof(buf) - 1;

      // Copy du lieu nguyen ban tu mbuf ra bo dem tam
      int rc = os_mbuf_copydata(ctxt->om, 0, len, buf);
      if(rc != 0) return BLE_ATT_ERR_UNLIKELY;

      // Kich hoat callback bao
      if(g_peripheral_srv_write_cb != NULL) g_peripheral_srv_write_cb(attr_handle, buf, len);

      return 0;
    }
    return BLE_ATT_ERR_UNLIKELY;
  }
  return BLE_ATT_ERR_UNLIKELY;
}

/*-----------------------------------------------------------*/

/* Mang cac dinh nghia cau truc co so du lieu GATT Table noi bo (service definition) */
static const struct ble_gatt_svc_def g_peripheral_svcs[] = {
  {
    .type = BLE_GATT_SVC_TYPE_PRIMARY,
    .uuid = &g_peripheral_svc_uuid.u,
    .characteristics = (struct ble_gatt_chr_def[]){
      {
        .uuid = &g_peripheral_tx_uuid.u,
        .access_cb = btcfg_peripheral_srv_internal_gatt_access_cb,
        .val_handle = &g_btcfg_peripheral_ctx.tx_attr_handle,
        .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY, // Hai vai tro la Read + Notify
      },
      {
        .uuid = &g_peripheral_rx_uuid.u,
        .access_cb = btcfg_peripheral_srv_internal_gatt_access_cb,
        .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
      },
      {0} // Khoa bien mang Characteristic
    }
  },
  {0} // Khoa bien mang Services
};

/* Ham tra ve con tro tro den dia chi phan tu dau tien cua mang g_peripheral_svcs */
const struct ble_gatt_svc_def *btcfg_peripheral_gatt_svcs_get(void){
  return g_peripheral_svcs;
}

/* ============================ GATT CLIENT INTERNAL CALLBACK HELPER ============================ */

/**
 * @note [PERIPHERAL] Callback don nhan va giai ma mang du lieu phan hoi sau khi ket thuc lenh Read Request tu GATT Client ra GATT Server tu xa.
 * Ham callback nay se duoc truyen vao ben trong ham thuc hien Read-Request den GATT Server
 */
static int btcfg_peripheral_clt_internal_read_rsp_handle(uint16_t conn_handle, const struct ble_gatt_error *error, struct ble_gatt_attr *attr, void *arg){
  (void)arg;
  uint16_t error_code = error->status;

  if(error_code != 0){
    BTCFG_LOGE("[PERIPHERAL] Client read failed, status=%u", error_code);
    return 0;
  }

  if(attr != NULL){
    uint16_t len = OS_MBUF_PKTLEN(attr->om);
    uint8_t buf[LOCAL_SERVER_BUF_SIZE];
    if(len >= sizeof(buf)) len = sizeof(buf) - 1;

    if(os_mbuf_copydata(attr->om, 0, len, buf) == 0) BTCFG_LOGI("[PERIPHERAL] Client read OK, len=%u", len);
    if(g_peripheral_clt_read_rsp_cb != NULL) g_peripheral_clt_read_rsp_cb(conn_handle, error_code, attr->handle, buf, len);
  }
  else if(g_peripheral_clt_read_rsp_cb != NULL) g_peripheral_clt_read_rsp_cb(conn_handle, error_code, 0, NULL, 0);
  return 0;
}

/*-----------------------------------------------------------*/

/**
 * @note [PERIPHERAL] Callback don nhan co bao trang thai phan hoi (Status Code) sau khi GATT Client push goi tin Write Request ra GATT Server tu xa.
 * Ham callback nay se duoc truyen vao ben trong ham thuc hien Write-Request den GATT Server
 */
static int btcfg_peripheral_clt_internal_write_handle(uint16_t conn_handle, const struct ble_gatt_error *error, struct ble_gatt_attr *attr, void *arg){
  (void)arg;
  BTCFG_LOGI("[PERIPHERAL] [Client Write Callback] conn_handle=%u, status=%d", conn_handle, error->status);
  return 0;
}

/* ============================ PUBLIC GATT SERVER API ============================ */

btcfg_err_t btcfg_peripheral_srv_set_characteristic_value(uint16_t attr_handle, const uint8_t *data, uint16_t len){
  if(!g_btcfg_peripheral_ctx.is_initialized) return BTCFG_ERR_NOT_READY;
  if(data == NULL || len > LOCAL_SERVER_BUF_SIZE) return BTCFG_ERR_INVALID_ARG;

  // BTCFG_LOCK(g_btcfg_mutex_handle);
  memcpy(s_peripheral_srv_local_data, data, len);
  s_peripheral_srv_local_len = len;
  // BTCFG_UNLOCK(g_btcfg_mutex_handle);

  return BTCFG_OK;
}

/*-----------------------------------------------------------*/

btcfg_err_t btcfg_peripheral_srv_notify_characteristic(uint16_t attr_handle, const uint8_t *data, uint16_t len){
  if(!g_btcfg_peripheral_ctx.is_initialized) return BTCFG_ERR_NOT_READY;
  if(data == NULL || len > LOCAL_SERVER_BUF_SIZE) return BTCFG_ERR_INVALID_ARG;
  if(!btcfg_peripheral_is_connected()) return BTCFG_ERR_NO_CONN;
  if(!g_btcfg_peripheral_ctx.srv_notify_enabled) return BTCFG_ERR_FAIL;

  /* 1. Cap nhat du lieu vao kho RAM cuc bo truoc */
  btcfg_err_t err = btcfg_peripheral_srv_set_characteristic_value(attr_handle, data, len);
  if(err != BTCFG_OK) return BTCFG_ERR_FAIL;

  /* 2. Tao mbuf tu mang tho phang de ghim cung chieu dai du lieu nhi phan ban ra khong trung */
  struct os_mbuf *om = ble_hs_mbuf_from_flat(data, len);
  if(om == NULL) return BTCFG_ERR_FAIL;
  
  // Su dung ham ble_gatts_notify_custom cho phep chi dinh chinh xac so luong bytes nhi phan (len)
  int rc = ble_gatts_notify_custom(g_btcfg_peripheral_ctx.conn_handle, attr_handle, om);
  return (rc == 0) ? BTCFG_OK : BTCFG_ERR_FAIL;
}

/*-----------------------------------------------------------*/

__attribute__((unused))
btcfg_err_t btcfg_peripheral_srv_send_text(uint16_t attr_handle, const char *text){
  if(!g_btcfg_peripheral_ctx.is_initialized) return BTCFG_ERR_NOT_READY;
  if(text == NULL) return BTCFG_ERR_INVALID_ARG;
  return btcfg_peripheral_srv_notify_characteristic(attr_handle, (const uint8_t*)text, strlen(text));
}

/* ============================ PUBLIC GATT CLIENT API ============================ */

btcfg_err_t btcfg_peripheral_clt_read(uint16_t attr_handle){
  if(!g_btcfg_peripheral_ctx.is_initialized) return BTCFG_ERR_NOT_READY;
  if(!btcfg_peripheral_is_connected()) return BTCFG_ERR_NO_CONN;

  // Dang ky ham nhan phan hoi noi bo cua NimBLE Host, g_peripheral_clt_read_rsp_cb se duoc trigger len ung dung
  int rc = ble_gattc_read(g_btcfg_peripheral_ctx.conn_handle, attr_handle, btcfg_peripheral_clt_internal_read_rsp_handle, NULL);
  return (rc == 0) ? BTCFG_OK : BTCFG_ERR_FAIL;
}

/*-----------------------------------------------------------*/

btcfg_err_t btcfg_peripheral_clt_write(uint16_t attr_handle, const uint8_t *data, uint16_t len){
  if(!g_btcfg_peripheral_ctx.is_initialized) return BTCFG_ERR_NOT_READY;
  if(!btcfg_peripheral_is_connected()) return BTCFG_ERR_NO_CONN;
  if(data == NULL || len == 0) return BTCFG_ERR_INVALID_ARG;

  int rc = ble_gattc_write_flat(g_btcfg_peripheral_ctx.conn_handle, attr_handle, data, len, btcfg_peripheral_clt_internal_write_handle, NULL);
  return (rc == 0) ? BTCFG_OK : BTCFG_ERR_FAIL;
}

/*-----------------------------------------------------------*/

btcfg_err_t btcfg_peripheral_clt_subscribe_notify(uint16_t attr_handle){
  if(!g_btcfg_peripheral_ctx.is_initialized) return BTCFG_ERR_NOT_READY;
  if(!btcfg_peripheral_is_connected()) return BTCFG_ERR_NO_CONN;

  // De kich hoat bat bat co Notify tu xa bang cach ghi gia tri 0x0001 (2 bytes) vao thanh ghi CCCD cua GATT Server
  // Sau khi subcribe notify thanh cong bang Write xong, ham callback internal_write se dung de xu ly phan hoi Write-Request tu ben GATT Server
  // Dung de bao trang thai "Toi da ghi thanh cong 0x0001 vao thanh ghi CCCD tu xa roi nhe !"
  uint16_t cccd_value = 0x0001;
  int rc = ble_gattc_write_flat(g_btcfg_peripheral_ctx.conn_handle, attr_handle, &cccd_value, sizeof(cccd_value), btcfg_peripheral_clt_internal_write_handle, NULL);
  return (rc == 0) ? BTCFG_OK : BTCFG_ERR_FAIL;
}

/* ============================ CALLBACK FUCNTION REGISTER ============================ */

void btcfg_peripheral_srv_register_write_callback(btcfg_peripheral_srv_write_cb_t cb){
  g_peripheral_srv_write_cb = cb;
}

/*-----------------------------------------------------------*/

void btcfg_peripheral_srv_register_read_callback(btcfg_peripheral_srv_read_cb_t cb){
  g_peripheral_srv_read_cb = cb;
}

/*-----------------------------------------------------------*/

void btcfg_peripheral_clt_register_read_response_callback(btcfg_peripheral_clt_read_rsp_cb_t cb){
  g_peripheral_clt_read_rsp_cb = cb;
}

/*-----------------------------------------------------------*/

void btcfg_peripheral_clt_register_notify_event_callback(btcfg_peripheral_clt_notify_evt_cb_t cb){
  g_peripheral_clt_ntf_evt_cb = cb;
}

/*-----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif // __cplusplus