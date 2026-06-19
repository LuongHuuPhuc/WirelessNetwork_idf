/**
 * Header phuc vu Application Layer cho cac project khac muon include 
 * Chua thu vien cau hinh BLE wrapper btcfg va ham dinh nghia callback dung o Application Layer
 */
#ifndef BTCFG_INCLUDE_H
#define BTCFG_INCLUDE_H

#pragma once 

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "btcfg_general.h"

#if defined(BTCFG_PERIPHERAL_USING) && !defined(BTCFG_CENTRAL_USING)
#include "btcfg_peripheral_proc.h"
#elif defined(BTCFG_CENTRAL_USING) && !defined(BTCFG_PERIPHERAL_USING)
#include "btcfg_central_proc.h"
#else /* Dinh nghia ca 2 macro nay */
#error "Can only using 1 GAP method ! (Peripheral or Client)"
#endif // BTCFG_GAP_USING


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // BTCFG_INCLUDE_H