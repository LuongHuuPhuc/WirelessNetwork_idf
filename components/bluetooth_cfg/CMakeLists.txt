file(GLOB BTCFG_SOURCES 
  "src/*.c"
  "src/central/*.c"
  "src/peripheral/*.c"
)

idf_component_register(
  SRCS 
    ${BTCFG_SOURCES} # Khong can khai bao tung file nua

  INCLUDE_DIRS 
    "include" 
    "include/private"
  
  REQUIRES 
    nvs_flash bt
)