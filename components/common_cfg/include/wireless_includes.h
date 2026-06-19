/**
 * @file wireless_includes.h
 * @note File su dung macro de thuc hien include thu vien theo phuong thuc muon su dung 
 * Co the include tai cac file khac de su dung
 * @author LuongHuuPhuc
 */

#ifndef WIRELESS_INCLUDES_H
#define WIRELESS_INCLUDES_H

#include "wireless_macros.h"

/* Neu muon dung Bluetooth */
#if defined(BLUETOOTH_CFG_USING)
#include "btcfg.h"

/* Neu muon dung Wifi */
#elif defined(WIFI_CFG_USING)
#include "wificfg.hpp"

/* Neu muon thiet lap Web Server */
#elif defined(WEBSERVER_CFG_USING)
#include "webservercfg.h"

#else
  #warning "Must to define wireless network stack first"
#endif // WIRELESS_CFG_USING
#endif // WIRELESS_INCLUDES_H