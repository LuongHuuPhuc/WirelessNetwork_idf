/**
 * @file main.c
 */

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#include "stdio.h"
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "wireless_includes.h"

#ifdef BLUETOOTH_CFG_USING
#if defined(BTCFG_PERIPHERAL_USING)
  /* ... */
#elif defined(BTCFG_CENTRAL_USING)
  /* ... */
#endif // BTCFG_GAP_USING

void app_main(void){
#ifdef BTCFG_PERIPHERAL_USING

#endif // BTCFG_PERIPHERAL_USING

#ifdef BTCFG_CENTRAL_USING 
  /* ... */    
#endif //BTCFG_CENTRAL_USING
}

#elif defined(WIFI_CFG_USING)
  /* ... */
  void app_main(void){}
#elif defined(WEBSERVER_CFG_USING)
  /* ... */
  void app_main(void){}
#endif // CFG_USING

#ifdef __cplusplus
}
#endif //__cplusplus
