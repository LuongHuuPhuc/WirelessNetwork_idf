/**
 * @file btcfg_peripheral_proc.h
 * @brief Xu ly BLE cho GAP Peripheral (Slave) voi GATT la Server/Client Dual-Role
 * @author Luong Huu Phuc
 * @date 2026/03/03
 * 
 * @details 
 * ESP32 dong vai tro quang ba ket noi va cho phep thiet bi khac ket noi den (Peripheral).
 * O tang GATT, thiet bi ho tro Dual-Role:
 * 1. GATT Server: Luu co so du lieu thuoc tinh, cho phep thiet bi khac truy cap (Read/Write)
 *    hoac ESP32 chu dong push du lieu ra khong trung (Notification/Indication)
 * 2. GATT Client: Cho phep chu dong gui yeu cau doc/ghi (Read/Write Request)
 *    hoac tim kiem dich vu (Service Discovery) tren mot GATT Server khac
 */

#ifndef _BTCFG_PERIPHERAL_PROC_INCLUDE_H
#define _BTCFG_PERIPHERAL_PROC_INCLUDE_H

#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "btcfg_general.h"
#include "btcfg_gap_gatt.h"

#define DISCONNECT_TO_CENTRAL_FLAG    0xFF

extern btcfg_peripheral_connect_cb_t g_peripheral_connect_cb;
extern btcfg_peripheral_disconnect_cb_t g_peripheral_disconnect_cb;
extern btcfg_peripheral_srv_read_cb_t g_peripheral_srv_read_cb;
extern btcfg_peripheral_srv_write_cb_t g_peripheral_srv_write_cb;
extern btcfg_peripheral_clt_notify_evt_cb_t g_peripheral_clt_ntf_evt_cb;
extern btcfg_peripheral_clt_read_rsp_cb_t g_peripheral_clt_read_rsp_cb;

/* ============================ PUBLIC API ============================ */

/**
 * @brief Khoi tao tai nguyen BLE (Controller + Host + NimBLE)
 * Ham nay phai duoc goi truoc khi su dung cac API khac 
 * 
 * @param[in] device_name Ten thiet bi muon dat de Central co the tim
 */
btcfg_err_t btcfg_peripheral_init(const char *device_name);

/**
 * @brief Kiem tra trang thai ket noi cua Peripheral
 * 
 * @return
 * - true neu da co client ket noi
 * - false neu chua co client ket noi
 */
bool btcfg_peripheral_is_connected(void);

/**
 * @brief Lay connection handle cua ket noi hien tai
 */
uint16_t btcfg_peripheral_get_conn_handle(void);


/* ----------------- GAP BLOCK (Advertising) ----------------- */

/**
 * @brief Kich hoat phat song quang ba de cho phep Central tim thay va ket noi toi ESP32
 * 
 * @note 
 * - ESP32 phai duoc khoi tao truoc
 * - Ham nay thuong la non-blocking
 */
btcfg_err_t btcfg_peripheral_start_advertising(void);

/**
 * @brief Dung quang ba cua ESP32 Peripheral
 */
btcfg_err_t btcfg_peripheral_stop_advertising(void);

/*  ----------------- GATT SERVER API BLOCK (Cuc bo cua Peripheral) ----------------- */

/**
 * @brief Cap nhat cuc bo mang du lieu vao Characteristic cua GATT Server (Peripheral)
 * 
 * @param[in] attr_handle Handle thuoc tinh can ghi
 * @param[in] data Con tro den data can ghi
 * @param[in] len Do dai du lieu (bytes)
 * 
 * @note Ham nay chi cap nhat gia tri noi bo (local RAM), khong tu dong gui Notify toi Client
 */
btcfg_err_t btcfg_peripheral_srv_set_characteristic_value(uint16_t attr_handle, const uint8_t *data, uint16_t len);

/**
 * @brief Chu dong gui thong bao (Notify) du lieu cua Characteristic cua GATT Server toi GATT Client (Peripheral)
 * 
 * @param[in] attr_handle Handle thuoc tinh can gui
 * @param[in] data Con tro den data can gui
 * @param[in] len Do dai du lieu (bytes)
 * 
 * @note 
 * - Phai co Client ket noi truoc khi goi ham nay 
 * - Client phai da dang ky nhan Notify 
 */
btcfg_err_t btcfg_peripheral_srv_notify_characteristic(uint16_t attr_handle, const uint8_t *data, uint16_t len);

/**
 * @brief Cap nhat text/data va gui Notify ngay toi GATT Client (Demo)
 * @param[in] text Chuoi text can gui
 * @param[in] attr_handle
 */
__attribute__((unused))
btcfg_err_t btcfg_peripheral_srv_send_text(uint16_t attr_handle, const char *text);

/*  ----------------- GATT CLIENT API BLOCK (Tuong tac ra thiet bi tu xa) ----------------- */

/**
 * @brief Gui yeu cau chu dong doc gia tri (Read Request) tu mot Characteristic tren Server tu xa
 * 
 * @param[in] attr_handle Handle cua Attribute tren thiet bi Server tu xa can doc
 * 
 * @note Ket qua du lieu tra ve tu Server tu xa se duoc xu ly trong callback `clt_read_response`
 */
btcfg_err_t btcfg_peripheral_clt_read(uint16_t attr_handle);

/**
 * @brief Chu dong ghi mang du lieu (Write Resquest) vao 1 Characteristic tren Server tu xa
 */
btcfg_err_t btcfg_peripheral_clt_write(uint16_t attr_handle, const uint8_t *data, uint16_t len);

/**
 * @brief Dang ky nhan Notify du lieu chu dong (Subscribe Notify) tu Characteristics cua Server tu xa
 * 
 * @param[in] attr_handle Handle co CCCD cua Characteristics ho tro thuoc tinh NOTIFY
 * 
 * @note 
 * - Charateristics phai hoi tro thuoc tinh NOTIFY
 * - Phai co ket noi truoc khi goi ham nay
 */
btcfg_err_t btcfg_peripheral_clt_subscribe_notify(uint16_t attr_handle);

/* ============================ CALLBACK FUCNTION REGISTER ============================ */

/**
 * @brief Dang ky Callback khi Peripheral ket noi thanh cong (Application Layer)
 * @param cb Ham callback do nguoi dung cung cap
 */
__attribute__((weak)) void btcfg_peripheral_register_connect_callback(btcfg_peripheral_connect_cb_t cb);

/**
 * @brief Dang ky Callback khi Peripheral bi ngat ket noi (Application Layer)
 * @param cb Ham callback do nguoi dung cung cap
 */
__attribute__((weak)) void btcfg_peripheral_register_disconnect_callback(btcfg_peripheral_disconnect_cb_t cb);

/* Server callback registers Application Layer (Danh cho viec nhan tuong tac tu thiet bi ngoai vao RAM ESP32) */
__attribute__((weak)) void btcfg_peripheral_srv_register_write_callback(btcfg_peripheral_srv_write_cb_t cb);
__attribute__((weak)) void btcfg_peripheral_srv_register_read_callback(btcfg_peripheral_srv_read_cb_t cb);

/* Client callback registers Application Layer (Danh cho viec ESP32 nhan phan hoi tu thiet bi Server tu xa tra ve)*/
__attribute__((weak)) void btcfg_peripheral_clt_register_read_response_callback(btcfg_peripheral_clt_read_rsp_cb_t cb);
__attribute__((weak)) void btcfg_peripheral_clt_register_notify_event_callback(btcfg_peripheral_clt_notify_evt_cb_t cb);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _BTCFG_PERIPHERAL_PROC_INCLUDE_H