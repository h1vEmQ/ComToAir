/**
 * @file config.h
 * @brief Конфигурационные параметры проекта ComToAir
 */

#ifndef CONFIG_H
#define CONFIG_H

// Конфигурация UART для USB-UART преобразователя (PL2303TA)
// D0 и D1 на XIAO ESP32-C6 соответствуют GPIO 0 и GPIO 1
// Подключение: Белый провод (TX USB-UART) -> D0 (GPIO 0)
//              Зеленый провод (RX USB-UART) -> D1 (GPIO 1)
// ВАЖНО: D0 = GPIO 0 (TX на ESP32), D1 = GPIO 1 (RX на ESP32)
// Поэтому: Белый (TX USB-UART) -> D1 (RX ESP32, GPIO 1)
//          Зеленый (RX USB-UART) -> D0 (TX ESP32, GPIO 0)
// Но если подключено: Белый -> D0, Зеленый -> D1, то нужно поменять местами в коде
#define UART_NUM            UART_NUM_1
#define UART_TX_PIN         GPIO_NUM_0  // D0 - передача данных в USB-UART (зеленый подключен сюда)
#define UART_RX_PIN         GPIO_NUM_1  // D1 - прием данных от USB-UART (белый подключен сюда)
#define UART_BUF_SIZE       1024
#define UART_BAUD_RATE      115200

// Конфигурация WiFi
#define WIFI_SSID_DEFAULT   "ComToAir_AP"
#define WIFI_PASS_DEFAULT   "12345678"
#define WIFI_MAX_CONN       4
#define WIFI_CHANNEL        1

// Конфигурация веб-сервера
#define WEB_SERVER_PORT     80
#define WEB_SERVER_MAX_URI_LEN 512

// Размеры буферов
#define DATA_BUFFER_SIZE    2048
#define JSON_BUFFER_SIZE    512

// Таймауты (в миллисекундах)
#define UART_READ_TIMEOUT   20
#define WIFI_RETRY_TIMEOUT  5000

#endif // CONFIG_H

