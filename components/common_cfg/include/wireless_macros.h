/**
 * @file wireless_macros.h
 * @author LuongHuuPhuc
 * @brief File khai bao macro, bien chung,... cho folder Wireless_network_test
 */

#ifndef _WIRELESS_MACROS_H
#define _WIRELESS_MACROS_H

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#pragma once 

#include "stdio.h"
#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LED_GPIO_TEST   GPIO_NUM_2 // Led blink demo

/* Lua chon phuong thuc ket noi (Bluetooth, Wifi, Web Server) */
#define BLUETOOTH_CFG_USING   1     /* Su dung song BLE Peripheral/Client */
// #define WIFI_CFG_USING        1     /* Su dung mang Wifi */
// #define WEBSERVER_CFG_USING   1     /* Su dung giao thuc WebServer */

#if !defined(BLUETOOTH_CFG_USING) && !defined(WIFI_CFG_USING) && !defined(WEBSERVER_CFG_USING)
  #warning "Define wireless protocol first if wanna use it"
#endif // XXX_CFG_USING

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // _WIRELESS_MACROS_H