idf_component_register(
  SRCS 
    "main.c"

  INCLUDE_DIRS 
    "."
  
  REQUIRES 
    common_cfg bluetooth_cfg webserver_cfg wifi_cfg
)