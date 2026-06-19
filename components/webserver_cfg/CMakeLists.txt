idf_component_register(
  SRCS 
    "src/webservercfg.c"
                       
  INCLUDE_DIRS 
    "include"
                       
  REQUIRES 
    esp_https_server
)