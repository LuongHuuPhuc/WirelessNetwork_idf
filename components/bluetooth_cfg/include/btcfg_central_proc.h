/**
 * @file btcfg_central_proc.h
 * @brief Xu ly BLE cho GAP Central (Master) voi GATT la Server/Client Dual-Role
 * @author Luong Huu Phuc
 * @date 2026/03/03
 * 
 * @details
 * ESP32 dong vai tro khoi tao ket noi va chap nhan ket noi toi Slave (Central).
 * O tang GATT, thiet bi ho tro Dual-Role:
 * 1. GATT Server: Luu co so du lieu thuoc tinh, cho phep thiet bi khac truy cap (Read/Write)
 *    hoac ESP32 chu dong push du lieu ra khong trung (Notification/Indication)
 * 2. GATT Client: Cho phep chu dong gui yeu cau doc/ghi (Read/Write Request)
 *    hoac tim kiem dich vu (Service Discovery) tren mot GATT Server khac
 */

#ifndef _BTCFG_CENTRAL_PROC_INCLUDE_H
#define _BTCFG_CENTRAL_PROC_INCLUDE_H

#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "btcfg_general.h"
#include "btcfg_gap_gatt.h"

#define SCAN_DURATION_MS      5000

extern btcfg_central_connect_cb_t g_central_connect_cb;
extern btcfg_central_disconnect_cb_t g_central_disconnect_cb;
extern btcfg_central_srv_read_cb_t g_central_srv_read_cb;
extern btcfg_central_srv_write_cb_t g_central_srv_write_cb;
extern btcfg_central_clt_notify_evt_cb_t g_central_clt_ntf_evt_cb;
extern btcfg_central_clt_read_rsp_cb_t g_central_clt_read_rsp_cb;

/* ============================ PUBLIC API ============================ */

/**
 * @brief Khoi tao BLE (Controller + Host + NimBLE)
 * Ham nay phai duoc goi truoc khi su dung cac API khac 
 */
btcfg_err_t btcfg_central_init(void);

/**
 * @brief Kiem tra trang thai ket noi cua Central
 * 
 * @return
 * - true neu da co ket noi toi Peripheral
 * - false neu chua co ket noi
 */
bool btcfg_central_is_connected(void);

/**
 * @brief Tra ve connection handle cua ket noi hien tai
 * 
 * @return
 * - Handle cua ket noi hien tai neu da ket noi
 * - Gia tri invalid handle neu chua co ket noi 
 */
uint16_t btcfg_central_get_conn_handle(void);

/* ----------------- GAP BLOCK (Scanning & Connections) ----------------- */

/**
 * @brief Bat dau quet thiet bi BLE xung quanh 
 * 
 * @param[in] duration_ms thoi gian quet (ms)
 * 
 * @note 
 * - Ham khong chan (non-blocking)
 * - Ket qua scan se duoc xu ly thong qua su kien noi bo
 */
btcfg_err_t btcfg_central_start_scan(uint32_t duration_ms);

/**
 * @brief ESP32 Central dung quet thiet bi BLE xung quanh
 */
btcfg_err_t btcfg_central_stop_scan(void);

/**
 * @brief Central chu dong ket noi toi thiet bi BLE theo ten quang ba
 * 
 * @param[in] name Ten thiet bi quang ba (Advertising Name) muon ket noi den
 * 
 * @warning 
 * - Ten phai trung voi ten quang ba cua thiet bi 
 * - Thiet bi phai dang o trang thai co the ket noi
 */
btcfg_err_t btcfg_central_connect(const char *name);

/**
 * @brief Central chu dong ngat ket noi BLE hien tai 
 */
btcfg_err_t btcfg_central_disconnect(void);

/*  ----------------- GATT SERVER API BLOCK (Cuc bo cua Peripheral) ----------------- */

/**
 * @brief Cap nhat cuc bo mang du lieu vao Characteristic cua GATT Server (Central)
 */
btcfg_err_t btcfg_central_srv_set_characteristic_value(uint16_t attr_handle, const uint8_t *data, uint16_t len);

/**
 * @brief Chu dong gui thong bao (Notify) du lieu cua Characteristic cua GATT Server toi GATT Client (Central)
 */
btcfg_err_t btcfg_central_srv_notify_characteristic(uint16_t attr_handle, const uint8_t *data, uint16_t len);

/*  ----------------- GATT CLIENT API BLOCK (Tuong tac ra thiet bi tu xa)  ----------------- */

/**
 * @brief Gui yeu cau chu dong doc gia tri (Read Request) cua mot Characteristic tre Server tu xa
 * 
 * @param attr_handle Handle cua Attribute can doc
 * 
 * @note 
 * - Phai co ket noi truoc khi goi ham nay 
 * - Ket qua du lieu tra ve tu Server tu xa se duoc xu ly trong callback `clt_read_response`
 */
btcfg_err_t btcfg_central_clt_read(uint16_t attr_handle);

/**
 * @brief Chu dong ghi mang du lieu (Write Request) vao mot Characteristic tren Server tu xa
 * 
 * @param[in] attr_handle Handle cua Attribute can ghi 
 * @param[in] data Con tro den du lieu can ghi 
 * @param[in] len Do dai cua du lieu (byte)
 */
btcfg_err_t btcfg_central_clt_write(uint16_t attr_handle, const uint8_t *data, uint16_t len);

/**
 * @brief Dang ky nhan Notify du lieu chu dong (Subscribe Notify) tu Characteristics cua Server tu xa
 * 
 * @param[in] attr_handle Handle co CCCD cua Characteristics ho tro thuoc tinh NOTIFY
 * 
 * @note 
 * - Charateristics phai hoi tro thuoc tinh NOTIFY
 * - Phai co ket noi truoc khi goi ham nay
 */
btcfg_err_t btcfg_central_clt_subscribe_notify(uint16_t attr_handle);

/* ============================ CALLBACK FUCNTION REGISTER ============================ */

/**
 * @brief Dang ky Callback khi ESP32 Central ket noi thanh cong toi Peripheral
 * 
 * @param cb Ham callback do nguoi dung cung cap
 */
__attribute__((weak)) void btcfg_central_register_connect_callback(btcfg_central_connect_cb_t cb);

/**
 * @brief Dang ky Callback khi ngat ket noi BLE
 * 
 * @param cb Ham callback do nguoi dung cung cap
 */
__attribute__((weak)) void btcfg_central_register_disconnect_callback(btcfg_central_disconnect_cb_t cb);

/* Server callback registers (Danh cho viec nhan tuong tac tu thiet bi ngoai vao RAM ESP32) */
__attribute__((weak)) void btcfg_central_srv_register_write_callback(btcfg_central_srv_write_cb_t cb);
__attribute__((weak)) void  btcfg_central_srv_register_read_callback(btcfg_central_srv_read_cb_t cb);

/* Client callback registers (Danh cho viec ESP32 nhan phan hoi tu thiet bi Server tu xa tra ve)*/
__attribute__((weak)) void btcfg_central_clt_register_read_response_callback(btcfg_central_clt_read_rsp_cb_t cb);
__attribute__((weak)) void btcfg_central_clt_register_notify_event_callback(btcfg_central_clt_notify_evt_cb_t cb);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _BTCFG_CENTRAL_PROC_INCLUDE_H