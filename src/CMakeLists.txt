# CMakeLists.txt for ComToAir main component

idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../include"
    PRIV_REQUIRES driver nvs_flash esp_wifi esp_http_server esp_event
)
