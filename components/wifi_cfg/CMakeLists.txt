file(GLOB WIFICFG_SOURCES 
  "src/*.c" 
  "src/*.cpp"
)

idf_component_register(
  SRCS 
    ${WIFICFG_SOURCES}
  
  INCLUDE_DIRS 
    "include"
                       
  REQUIRES 
    esp_wifi
)