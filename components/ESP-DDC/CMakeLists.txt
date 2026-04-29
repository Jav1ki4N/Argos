# ====================================================
# ESP-DDC Component
# i4N@2026
# this file will guide cmake to properly build ESP-DDC
# ====================================================

# https://docs.espressif.com/projects/esp-idf/zh_CN/v6.0/esp32s3/api-guides/build-system.html#component-directories

idf_component_register(SRCS
                       INCLUDE_DIRS "include"
                       REQUIRES driver u8g2 esp_wifi nvs_flash esp_http_client esp_timer esp_netif 
                       )
