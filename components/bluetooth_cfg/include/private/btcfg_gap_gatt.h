/**
 * @file btcfg_gap_gatt.h
 * @brief File khai bao chi tiet cau truc noi bo (struct) chung cho toan bo GAP (Peripheral/Central) voi GATT Dual-Role (Server/Client)
 * va State Machine xu ly event Host Stack NimBLE
 * 
 * @warning
 * File chua cac dinh nghia phan vung nho tang thap (Low-level Abstraction)
 * Khong dung truc tiep tai Application layer
 * Chi luu hanh trong pham vi noi bo thu vien
 * 
 * @author Luong Huu Phuc
 * @date 2026/03/03
 */

#ifndef BTCFG_GAP_GATT_INCLUDE_H
#define BTCFG_GAP_GATT_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#pragma once 

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "host/ble_gap.h"

#define LOCAL_SERVER_BUF_SIZE     512  /* Kich thuoc buffer data dung cho GATT Server */

/** 
 * @brief May trang thai (State Machine) quan ly vong doi cua BLE
 * @enum btcfg_state_t
 * 
 * @details 
 * Giup quan ly ro rang vong doi BLE:
 *  - Tu chua khoi tao
 *  - Den Scanning
 *  - Ket noi 
 *  - Ngat ket noi
 * 
 * Viec su dung state machine giup:
 *  - Tranh goi API sai thoi diem 
 *  - De debug
 *  - De mo rong
 */
typedef enum {
  BTCFG_STATE_IDLE = 0,
  BTCFG_STATE_INITIALIZED,
  BTCFG_STATE_SCANNING,
  BTCFG_STATE_CONNECTING,
  BTCFG_STATE_CONNECTED,
  BTCFG_STATE_DISCONNECTED,
  BTCFG_STATE_ADVERTISING,
  BTCFG_STATE_ADV_STOPPED
} btcfg_state_t;

/* ============================ INTERNAL GAP CONTEXT STRUCT ============================ */

/**
 * @brief Cau truc luu trang thai noi bo cho vai tro GAP Central (Master)
 */
typedef struct {
  btcfg_state_t state;     /* !<Trang thai Core hien tai cua Central> */
  uint16_t conn_handle;    /* !<Dinh danh phien ket noi do Host cap> */
  uint16_t tx_attr_handle; /* !<Handle cuc bo dung de ban Notify du lieu neu Central la GATT Server> */
  bool is_initialized;     /* !<Co kiem tra khoi tao Driver Central> */
  bool is_scanning;        /* !<Co bao trang thai dang quet song danh rieng cho GAP Central> */
  bool srv_notify_enabled; /* !<Co CCCD: dung khi Central la GATT Server va co thiet bi subscribe notify>*/
  uint8_t own_addr_type;   /* !<Loai dia chi MAC cua ESP32 (Public/Randowm)>*/
} btcfg_central_context_t;

/**
 * @brief Cau truc luu trang thai noi bo cho vai tro GAP Peripheral (Slave)
 */
typedef struct {
  btcfg_state_t state;       /* !<Trang thai Core hien tai cua Peripheral> */
  uint16_t conn_handle;      /* !<Dinh danh phien ket noi do Host cap> */
  uint16_t tx_attr_handle;   /* !<Handle cuc bo dung de ban Notify du lieu neu Peripheral la GATT Server> */
  bool is_initialized;       /* !<Co kiem tra khoi tao Driver Peripheral> */
  bool is_advertising;       /* !<Co bao trang thai phat quang ba danh rieng cho GAP Peripheral> */
  bool srv_notify_enabled;   /* !<Co CCCD: dung khi Peripheral la GATT Server va co thiet bi subscribe notify> */
  uint8_t own_addr_type;     /* !<Loai dia chi MAC cua ESP32 (Public/Random)> */ 
} btcfg_peripheral_context_t;

// Bien global chua cac thanh phan contex quan ly ket noi
extern btcfg_central_context_t g_btcfg_central_ctx;
extern btcfg_peripheral_context_t g_btcfg_peripheral_ctx;

/* ============================ INTERNAL CALLBACK TYPES ============================ */
/**
 * (Cac ham su dung con tro ham) 
 * - Khi co su kien -> goi la ham cua nguoi dung
 * - Thu vien su dung callback vi BLE hoat dong theo co che bat dong bo (event-driven). 
 * Callback cho phep thu vien thong bao su kien (ket noi, ngat ket noi,...) ve ung dung va khong 
 * can block chuong trinh, dong thoi giup tach biet logic tang giao tiep va tang ung dung, lam 
 * cho thu vien linh hoat hon va tai su dung tot hon
 * App đăng ký callback 
 *           ↓
 * Thư viện nhận event BLE
 *           ↓
 * Thư viện gọi lại App
 *           ↓
 * App xử lý theo ý mình
 */

/* ------------------------------ FOR PERIPHERAL ------------------------------ */

/* ===== 1. GAP & Connection Callbacks ===== */

/**
 * @brief Callback duoc goi khi ket noi BLE duoc thiet lap thanh cong
 * @param[in] conn_handle Handle cua ket noi do BLE stack cap
 */
typedef void (*btcfg_peripheral_connect_cb_t)(uint16_t conn_handle);

/**
 * @brief Callback khi ket noi bi ngat
 */
typedef void (*btcfg_peripheral_disconnect_cb_t)(void);

/* ===== 2. GATT SERVER Callbacks (ESP32 Peripheral lam GATT Server de cac thiet bi khac truy cap) ===== */

/**
 * @brief Callback xu ly su kien khi GATT Client ben ngoai chu dong ghi du lieu (Write Request) 
 * vao 1 Characteristic tren ESP32 Peripheral (GATT Server)
 * 
 * @param attr_handle Handle cua Attribute bi ghi
 * @param data Con tro toi vung du lieu nhan duoc
 * @param length Do dai du lieu (byte)
 */
typedef void (*btcfg_peripheral_srv_write_cb_t)(uint16_t attr_handle, const uint8_t *data, uint16_t length);

/**
 * @brief Callback xu ly su kien khi GATT Client ben ngoai chu doc gui yeu cau doc (Read Request)
 * gia tri cua mot Characteristic tren ESP32 Peripheral (GATT Server)
 * 
 * @param[in] attr_handle Handle vao Attribute can doc
 * @param[in] conn_handle Handle cua ket noi hien tai
 */
typedef void (*btcfg_peripheral_srv_read_cb_t)(uint16_t attr_handle, uint16_t conn_handle);

/* ===== 3. GATT CLIENT Callbacks (ESP32 Peripheral lam GATT Client va chu dong tuong tac voi Server tu xa) ===== */

/**
 * @brief Callback xu ly su kien khi nhan du lieu phan hoi nguoc lai khi ESP32 Peripheral (GATT Client) gui lenh Read toi Server tu xa
 * 
 * @param[in] conn_handle Handle cua ket noi hien tai
 * @param[in] err_code Ma loi BLE tra ve (0 neu doc thanh cong)
 * @param[in] attr_handle Handle cua thuoc tinh tu xa vua doc
 * @param[in] data Con tro toi du lieu nhan duoc
 * @param[in] len Do dai du lieu (bytes)
 * 
 * @note Khi o GATT Client, khi phat lenh Read-Request den GATT Server, no dang gui di 1 yeu cau de doi du lieu tu Server.
 * Luc nay ham callback nay bat buoc phai co cau truc nhan con tro du lieu `struct ble_gatt_attr *attr`.
 * Nhiem vu cua no la hứng mang bytes tho tu bo dem `attr->om` do de ve ung dung xu ly. The nen Read-Request xong can phai 
 * co callback de nhan du lieu rieng tu GATT Server gui ve
 * \note Con Write-Request khong can write_rsp nhu Read-Request vi no chi thuc hien ghi chu khong can nhan lai bat cu 
 * bytes nao ca ngoai Status Code (ma trang thai) bao thanh cong hay chua.
 */
typedef void (*btcfg_peripheral_clt_read_rsp_cb_t)(uint16_t conn_handle, uint16_t err_code, uint16_t attr_handle, const uint8_t *data, uint16_t len);

/**
 * @brief Callback xu ly su kien khi GATT Server tu xa chu dong ban Notify toi ESP32 Peripheral (GATT Client)
 * 
 * @param[in] conn_handle Handle cua ket noi hien tai
 * @param[in] attr_handle Handle cua thuoc tinh tu xa phat thong bao
 * @param[in] data Con tro toi mang du lieu thong bao
 * @param[in] len Do dai du lieu (bytes)
 */
typedef void (*btcfg_peripheral_clt_notify_evt_cb_t)(uint16_t conn_handle, uint16_t attr_handle, const uint8_t *data, uint16_t len);

/* ------------------------------ FOR CENTRAL ------------------------------ */

/* ===== 1. GAP & Connection Callbacks ===== */

/**
 * @brief Callback duoc goi khi ket noi BLE duoc thiet lap thanh cong
 * @param[in] conn_handle Handle cua ket noi do BLE stack cap
 */
typedef void (*btcfg_central_connect_cb_t)(uint16_t conn_handle);

/**
 * @brief Ham Callback khi ket noi bi ngat
 */
typedef void (*btcfg_central_disconnect_cb_t)(void);

/* ===== 2. GATT SERVER Callbacks (ESP32 Central lam GATT Server cho cac BLE khac doc/ghi du lieu) ===== */

/**
 * @brief Callback xu ly su kien khi GATT Client ben ngoai chu dong ghi du lieu (Write Request) 
 * vao 1 Characteristic tren ESP32 Central (GATT Server)
 * 
 * @param attr_handle Handle cua Attribute bi ghi
 * @param data Con tro toi vung du lieu nhan duoc
 * @param length Do dai du lieu (byte)
 */
typedef void (*btcfg_central_srv_write_cb_t)(uint16_t attr_handle, const uint8_t *data, uint16_t len);

/**
 * @brief Callback xu ly su kien khi GATT Client ben ngoai chu doc gui yeu cau doc (Read Request)
 * gia tri cua mot Characteristic tren ESP32 Central (GATT Server)
 * 
 * @param[in] attr_handle Handle vao Attribute can doc
 * @param[in] conn_handle Handle cua ket noi hien tai
 */
typedef void (*btcfg_central_srv_read_cb_t)(uint16_t attr_handle, uint16_t conn_handle);

/* ===== 3. GATT CLIENT Callbacks (ESP32 Central lam GATT Client, chu dong quan ly cac Service tu xa) ===== */

/**
 * @brief Callback xu ly su kien khi nhan du lieu phan hoi nguoc lai khi ESP32 Central (GATT Client) gui lenh Read toi Server tu xa
 * 
 * @param[in] conn_handle Handle cua ket noi hien tai
 * @param[in] err_code Ma loi BLE tra ve (0 neu doc thanh cong)
 * @param[in] attr_handle Handle cua thuoc tinh tu xa vua doc
 * @param[in] data Con tro toi du lieu nhan duoc
 * @param[in] len Do dai du lieu (bytes)
 * 
 * @note Khi o GATT Client, khi phat lenh Read-Request den GATT Server, no dang gui di 1 yeu cau de doi du lieu tu Server.
 * Luc nay ham callback nay bat buoc phai co cau truc nhan con tro du lieu `struct ble_gatt_attr *attr`.
 * Nhiem vu cua no la hứng mang bytes tho tu bo dem `attr->om` do de ve ung dung xu ly. The nen Read-Request xong can phai 
 * co callback de nhan du lieu rieng tu GATT Server gui ve. 
 * \note Con Write-Request khong can write_rsp nhu Read-Request vi no chi thuc hien ghi chu khong can nhan lai bat cu 
 * bytes nao ca ngoai Status Code (ma trang thai) bao thanh cong hay chua.
 */
typedef void (*btcfg_central_clt_read_rsp_cb_t)(uint16_t conn_handle, uint16_t err_code, uint16_t attr_handle, const uint8_t *data, uint16_t len);

/**
 * @brief Callback xu ly su kien khi GATT Server tu xa chu dong ban Notify toi ESP32 Central (GATT Client)
 * 
 * @param[in] conn_handle Handle cua ket noi hien tai
 * @param[in] attr_handle Handle cua thuoc tinh tu xa phat thong bao
 * @param[in] data Con tro toi mang du lieu thong bao
 * @param[in] len Do dai du lieu (bytes)
 */
typedef void (*btcfg_central_clt_notify_evt_cb_t)(uint16_t conn_handle, uint16_t attr_handle, const uint8_t *data, uint16_t len);

/* ============================ INTERNAL GAP EVENT HANDLER FUNCTION ============================ */

/**
 * @note 
 * -> GATT Client/Server tuong tac voi nhau bang luong du lieu Chu dong phat song lien tuc nhu (Subscribe/Notify RX) qua khong trung
 * -> Bat buoc phai cau hinh don ngat ngay tai GAP event handler de bat kip toc do cua mach Radio phan cung.
 */

/**
 * @brief State Machine xu ly toan bo vong lap su kien GAP voi vai tro Central
 */
int btcfg_central_gap_event(struct ble_gap_event *event, void *arg);

/**
 * @brief State Machine xu ly toan bo vong lap su kien GAP voi vai tro Perpipheral
 */
int btcfg_peripheral_gap_event(struct ble_gap_event *event, void *arg);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // BTCFG_INTERNAL_INCLUDE_H