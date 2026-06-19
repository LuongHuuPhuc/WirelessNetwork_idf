/**
 * @file wifi_cfg.hpp
 * @note Header file module hoa API driver Wifi cua ESP-IDF thanh cac ham de dung
 * @author Luong Huu Phuc
 * @date 2025/10/02
 */

#ifndef WIFI_CFG_H
#define WIFI_CFG_H

#pragma once

#ifdef __cplusplus
  #define EXTERN_C extern "C"
#else 
  #define EXTERN_C extern
#endif //__cplusplus

#include <stdio.h>
#include <iostream>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_netif.h>
#include <esp_err.h>
#include <esp_log.h>
#include <freertos/event_groups.h>

#endif //WIFI_CFG_H