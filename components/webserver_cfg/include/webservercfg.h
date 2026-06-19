/**
 * @file Webserver_testcfg.h
 */

#ifndef WEBSERVER_TESTCFG_H
#define WEBSERVER_TESTCFG_H

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#pragma once

#include "stdio.h"
#include "stdlib.h"
#include "esp_https_server.h"
#include "esp_err.h"
#include "esp_log.h"

#ifndef MIN 
  #define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif //MIN

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //WEBSERVER_TESTCFG_H