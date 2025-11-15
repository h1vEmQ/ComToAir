/**
 * ComToAir - Устройство для приема данных по RS-232 и трансляции через WiFi
 * 
 * Основной файл приложения для Seeed Studio XIAO ESP32-C6
 * Framework: ESP-IDF
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_http_server.h"

static const char *TAG = "ComToAir";

// Конфигурация UART для USB-UART преобразователя (PL2303TA)
// D0 и D1 на XIAO ESP32-C6 соответствуют GPIO 0 и GPIO 1
// Подключение пользователя: Белый провод (TX USB-UART) -> D0 (GPIO 0)
//                           Зеленый провод (RX USB-UART) -> D1 (GPIO 1)
// ВАЖНО: Для правильной работы нужно перекрестное подключение:
//        TX USB-UART -> RX ESP32, RX USB-UART -> TX ESP32
// Текущее подключение: Белый (TX USB-UART) -> D0 (GPIO 0, TX ESP32) - НЕПРАВИЛЬНО!
//                     Зеленый (RX USB-UART) -> D1 (GPIO 1, RX ESP32) - НЕПРАВИЛЬНО!
// Решение: Поменять местами в коде, чтобы компенсировать неправильное подключение
#define UART_NUM            UART_NUM_1
#define UART_TX_PIN         GPIO_NUM_0  // D0 - передача данных (зеленый подключен сюда)
#define UART_RX_PIN         GPIO_NUM_1  // D1 - прием данных (белый подключен сюда)
#define UART_BUF_SIZE       1024
#define BUF_SIZE            (UART_BUF_SIZE)

// Конфигурация WiFi (по умолчанию)
#define WIFI_SSID           "ComToAir_AP"
#define WIFI_PASS           "12345678"
#define WIFI_MAXIMUM_RETRY  5

// Буфер для данных UART
static uint8_t uart_buffer[BUF_SIZE];
// Статистика UART
static size_t uart_total_received = 0;

/**
 * Инициализация UART для работы с USB-UART преобразователем
 */
void init_uart(void)
{
    // Проверяем состояние пинов перед инициализацией
    gpio_reset_pin(UART_RX_PIN);
    gpio_reset_pin(UART_TX_PIN);
    
    // Настраиваем пины как GPIO для диагностики
    gpio_set_direction(UART_RX_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(UART_TX_PIN, GPIO_MODE_OUTPUT);
    
    // Проверяем начальное состояние RX пина
    int rx_level = gpio_get_level(UART_RX_PIN);
    ESP_LOGI(TAG, "GPIO%d (RX/A0) initial level: %d", UART_RX_PIN, rx_level);
    
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    ESP_LOGI(TAG, "Installing UART driver...");
    esp_err_t ret = uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UART driver install failed: %s", esp_err_to_name(ret));
        return;
    }
    
    ESP_LOGI(TAG, "Configuring UART parameters...");
    ret = uart_param_config(UART_NUM, &uart_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UART param config failed: %s", esp_err_to_name(ret));
        return;
    }
    
    ESP_LOGI(TAG, "Setting UART pins: TX=GPIO%d, RX=GPIO%d", UART_TX_PIN, UART_RX_PIN);
    ret = uart_set_pin(UART_NUM, UART_TX_PIN, UART_RX_PIN, 
                       UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UART set pin failed: %s", esp_err_to_name(ret));
        return;
    }
    
    ESP_LOGI(TAG, "UART initialized: RX=GPIO%d (A0), TX=GPIO%d (A1), Baud=115200", 
             UART_RX_PIN, UART_TX_PIN);
    
    // Очистка буферов
    uart_flush_input(UART_NUM);
    uart_flush(UART_NUM);
    ESP_LOGI(TAG, "UART buffers flushed");
    
    // Проверяем состояние пинов после инициализации
    rx_level = gpio_get_level(UART_RX_PIN);
    ESP_LOGI(TAG, "GPIO%d (RX/A0) level after init: %d", UART_RX_PIN, rx_level);
    
    // Тестовая отправка для проверки связи
    const char* test_msg = "ComToAir UART Test\r\n";
    int bytes_written = uart_write_bytes(UART_NUM, test_msg, strlen(test_msg));
    ESP_LOGI(TAG, "Test message sent: %d bytes written", bytes_written);
    
    // Небольшая задержка для стабилизации
    vTaskDelay(100 / portTICK_PERIOD_MS);
    
    // Проверяем, есть ли что-то в буфере после отправки
    size_t buffered = 0;
    uart_get_buffered_data_len(UART_NUM, &buffered);
    ESP_LOGI(TAG, "Bytes in buffer after test send: %d", buffered);
}

/**
 * Задача для периодической отправки тестовых данных
 */
void uart_test_task(void *pvParameters)
{
    int test_counter = 0;
    while (1) {
        vTaskDelay(5000 / portTICK_PERIOD_MS);  // Каждые 5 секунд
        
        char test_buf[64];
        int len = snprintf(test_buf, sizeof(test_buf), "Test %d\r\n", ++test_counter);
        int written = uart_write_bytes(UART_NUM, test_buf, len);
        ESP_LOGI(TAG, "Periodic test message sent: %d bytes (counter: %d)", written, test_counter);
        
        // Проверяем уровень RX пина
        int rx_level = gpio_get_level(UART_RX_PIN);
        ESP_LOGI(TAG, "GPIO%d (RX/A0) current level: %d", UART_RX_PIN, rx_level);
    }
}

/**
 * Задача для мониторинга уровня RX пина (для диагностики)
 */
void uart_pin_monitor_task(void *pvParameters)
{
    int last_level = -1;
    int level_changes = 0;
    int samples = 0;
    
    ESP_LOGI(TAG, "UART pin monitor task started");
    
    while (1) {
        // Читаем уровень RX пина напрямую через GPIO
        int current_level = gpio_get_level(UART_RX_PIN);
        samples++;
        
        // Считаем изменения уровня (признак активности)
        if (current_level != last_level) {
            level_changes++;
            last_level = current_level;
            ESP_LOGI(TAG, "RX pin level changed to: %d (changes: %d)", current_level, level_changes);
        }
        
        // Каждые 1000 сэмплов выводим статистику
        if (samples % 1000 == 0) {
            ESP_LOGI(TAG, "Pin monitor: samples=%d, level_changes=%d, current_level=%d", 
                     samples, level_changes, current_level);
            
            if (level_changes == 0) {
                ESP_LOGW(TAG, "WARNING: RX pin level never changed! Check wiring!");
            }
        }
        
        vTaskDelay(10 / portTICK_PERIOD_MS);  // Очень частое опрашивание для обнаружения изменений
    }
}

/**
 * Задача для чтения данных из UART
 */
void uart_read_task(void *pvParameters)
{
    size_t total_received = 0;
    int read_attempts = 0;
    int consecutive_zeros = 0;
    ESP_LOGI(TAG, "UART read task started");
    
    while (1) {
        read_attempts++;
        
        // Проверяем уровень RX пина перед чтением
        int rx_level = gpio_get_level(UART_RX_PIN);
        
        // Проверяем наличие данных в буфере
        size_t buffered_size = 0;
        uart_get_buffered_data_len(UART_NUM, &buffered_size);
        
        if (buffered_size > 0) {
            ESP_LOGI(TAG, "*** Data available in buffer: %d bytes, RX pin level: %d ***", 
                     buffered_size, rx_level);
            consecutive_zeros = 0;
        } else {
            consecutive_zeros++;
        }
        
        // Периодически выводим диагностику
        if (read_attempts % 500 == 0) {
            ESP_LOGI(TAG, "UART status: buffered=%d, RX_level=%d, attempts=%d, total_received=%d, consecutive_zeros=%d", 
                     buffered_size, rx_level, read_attempts, total_received, consecutive_zeros);
            
            // Предупреждение, если долго нет данных
            if (consecutive_zeros > 1000 && total_received == 0) {
                ESP_LOGW(TAG, "WARNING: No data received for a long time!");
                ESP_LOGW(TAG, "Check: 1) Wiring (white->A0, green->A1) 2) COM port settings 3) Data is being sent");
            }
        }
        
        // Читаем данные с коротким таймаутом для более быстрой реакции
        int len = uart_read_bytes(UART_NUM, uart_buffer, BUF_SIZE - 1, 50 / portTICK_PERIOD_MS);
        
        if (len > 0) {
            uart_buffer[len] = '\0';
            total_received += len;
            uart_total_received += len;  // Обновляем глобальный счетчик
            consecutive_zeros = 0;
            
            // Логируем полученные данные
            ESP_LOGI(TAG, "*** RECEIVED %d bytes (total: %d) ***", len, total_received);
            
            // Выводим данные в hex для отладки
            char hex_str[512];
            int hex_len = 0;
            for (int i = 0; i < len && i < 50 && hex_len < 500; i++) {
                hex_len += snprintf(hex_str + hex_len, sizeof(hex_str) - hex_len, 
                                   "%02X ", uart_buffer[i]);
            }
            ESP_LOGI(TAG, "Hex: %s", hex_str);
            
            // Выводим как текст (если это печатные символы)
            bool is_printable = true;
            for (int i = 0; i < len; i++) {
                if (uart_buffer[i] < 32 || uart_buffer[i] > 126) {
                    if (uart_buffer[i] != '\r' && uart_buffer[i] != '\n' && 
                        uart_buffer[i] != '\t') {
                        is_printable = false;
                        break;
                    }
                }
            }
            
            if (is_printable) {
                ESP_LOGI(TAG, "Text: %s", (char*)uart_buffer);
            } else {
                ESP_LOGI(TAG, "Binary data received (first 20 bytes shown)");
            }
        } else if (len == 0) {
            // Таймаут - нет данных, но это нормально
        } else {
            // Ошибка чтения
            ESP_LOGE(TAG, "UART read error: %d", len);
        }
        
        vTaskDelay(20 / portTICK_PERIOD_MS);  // Более частое опрашивание
    }
}

/**
 * Обработчик событий WiFi
 */
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "WiFi disconnected, retrying...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
    }
}

/**
 * Инициализация WiFi в режиме точки доступа
 */
void init_wifi_ap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {};
    strncpy((char*)wifi_config.ap.ssid, WIFI_SSID, sizeof(wifi_config.ap.ssid) - 1);
    wifi_config.ap.ssid_len = strlen(WIFI_SSID);
    strncpy((char*)wifi_config.ap.password, WIFI_PASS, sizeof(wifi_config.ap.password) - 1);
    wifi_config.ap.max_connection = 4;
    wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi AP initialized. SSID:%s password:%s", WIFI_SSID, WIFI_PASS);
}

/**
 * HTTP обработчик для главной страницы
 */
static esp_err_t root_get_handler(httpd_req_t *req)
{
    const char* html = 
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<title>ComToAir</title>"
        "<meta charset='UTF-8'>"
        "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
        "<style>"
        "body { font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; }"
        ".container { max-width: 800px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }"
        "h1 { color: #333; }"
        ".status { padding: 10px; margin: 10px 0; border-radius: 4px; background: #e3f2fd; }"
        ".data { padding: 15px; margin: 10px 0; border: 1px solid #ddd; border-radius: 4px; background: #fafafa; font-family: monospace; }"
        "button { padding: 10px 20px; background: #2196F3; color: white; border: none; border-radius: 4px; cursor: pointer; }"
        "button:hover { background: #1976D2; }"
        "</style>"
        "</head>"
        "<body>"
        "<div class='container'>"
        "<h1>ComToAir - RS-232 to WiFi Bridge</h1>"
        "<div class='status'>"
        "<h2>Статус устройства</h2>"
        "<p>WiFi: Подключено</p>"
        "<p>RS-232: Активен</p>"
        "</div>"
        "<div class='data'>"
        "<h2>Данные RS-232</h2>"
        "<div id='data'>Ожидание данных...</div>"
        "</div>"
        "<button onclick='refreshData()'>Обновить</button>"
        "<script>"
        "function refreshData() {"
        "  fetch('/api/data')"
        "    .then(response => response.json())"
        "    .then(data => {"
        "      document.getElementById('data').textContent = data.data || 'Нет данных';"
        "    });"
        "}"
        "setInterval(refreshData, 1000);"
        "</script>"
        "</body>"
        "</html>";
    
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);
}

/**
 * HTTP обработчик для API данных
 */
static esp_err_t api_data_get_handler(httpd_req_t *req)
{
    char response[1024];
    size_t data_len = strlen((char*)uart_buffer);
    
    // Получаем текущий размер буфера
    size_t buffered = 0;
    uart_get_buffered_data_len(UART_NUM, &buffered);
    
    // Ограничиваем длину данных для безопасности
    if (data_len > 200) {
        data_len = 200;
    }
    char safe_data[201];
    strncpy(safe_data, (char*)uart_buffer, data_len);
    safe_data[data_len] = '\0';
    
    // Экранируем специальные символы для JSON
    char escaped_data[401];
    int j = 0;
    for (int i = 0; i < data_len && j < 400; i++) {
        if (safe_data[i] == '"') {
            escaped_data[j++] = '\\';
            escaped_data[j++] = '"';
        } else if (safe_data[i] == '\\') {
            escaped_data[j++] = '\\';
            escaped_data[j++] = '\\';
        } else if (safe_data[i] == '\n') {
            escaped_data[j++] = '\\';
            escaped_data[j++] = 'n';
        } else if (safe_data[i] == '\r') {
            escaped_data[j++] = '\\';
            escaped_data[j++] = 'r';
        } else if (safe_data[i] >= 32 && safe_data[i] <= 126) {
            escaped_data[j++] = safe_data[i];
        } else {
            // Пропускаем непечатные символы
        }
    }
    escaped_data[j] = '\0';
    
    snprintf(response, sizeof(response), 
        "{\"data\":\"%s\",\"length\":%zu,\"buffered\":%zu,\"total_received\":%zu}", 
        escaped_data, strlen((char*)uart_buffer), buffered, uart_total_received);
    
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
}

/**
 * HTTP обработчик для статуса UART
 */
static esp_err_t api_uart_status_handler(httpd_req_t *req)
{
    char response[512];
    size_t buffered = 0;
    uart_get_buffered_data_len(UART_NUM, &buffered);
    
    snprintf(response, sizeof(response), 
        "{\"uart_active\":true,\"rx_pin\":%d,\"tx_pin\":%d,\"baud_rate\":115200,"
        "\"buffered_bytes\":%zu,\"total_received\":%zu,\"buffer_size\":%d}",
        UART_RX_PIN, UART_TX_PIN, buffered, uart_total_received, BUF_SIZE);
    
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
}

/**
 * Инициализация HTTP сервера
 */
httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    ESP_LOGI(TAG, "Starting web server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "Registering URI handlers");
        
        httpd_uri_t root = {
            .uri       = "/",
            .method    = HTTP_GET,
            .handler   = root_get_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &root);
        
        httpd_uri_t api_data = {
            .uri       = "/api/data",
            .method    = HTTP_GET,
            .handler   = api_data_get_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &api_data);
        
        httpd_uri_t api_uart_status = {
            .uri       = "/api/uart/status",
            .method    = HTTP_GET,
            .handler   = api_uart_status_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &api_uart_status);
        
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

/**
 * Главная функция приложения
 */
extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "ComToAir starting...");
    
    // Инициализация NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Инициализация UART
    init_uart();
    
    // Инициализация WiFi
    init_wifi_ap();
    
    // Запуск задачи чтения UART
    xTaskCreate(uart_read_task, "uart_read_task", 4096, NULL, 10, NULL);
    
    // Запуск задачи мониторинга уровня RX пина
    xTaskCreate(uart_pin_monitor_task, "uart_pin_monitor", 2048, NULL, 5, NULL);
    
    // Запуск тестовой задачи для периодической отправки данных
    xTaskCreate(uart_test_task, "uart_test_task", 2048, NULL, 5, NULL);
    
    // Запуск веб-сервера
    start_webserver();
    
    ESP_LOGI(TAG, "ComToAir initialized successfully");
}

