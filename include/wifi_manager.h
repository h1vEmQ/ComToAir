/**
 * @file wifi_manager.h
 * @brief Управление WiFi подключением
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Режим работы WiFi
 */
typedef enum {
    WIFI_MODE_STATION,     // Режим станции (подключение к сети)
    WIFI_MODE_AP,          // Режим точки доступа
    WIFI_MODE_APSTA        // Режим точка доступа + станция
} wifi_mode_t;

/**
 * @brief Структура конфигурации WiFi
 */
typedef struct {
    char ssid[32];          // SSID сети
    char password[64];       // Пароль сети
    wifi_mode_t mode;       // Режим работы
} wifi_config_t;

/**
 * @brief Статус подключения WiFi
 */
typedef enum {
    WIFI_STATUS_DISCONNECTED,
    WIFI_STATUS_CONNECTING,
    WIFI_STATUS_CONNECTED,
    WIFI_STATUS_ERROR
} wifi_status_t;

/**
 * @brief Инициализация WiFi
 * 
 * @return true при успешной инициализации, false в противном случае
 */
bool wifi_manager_init(void);

/**
 * @brief Настройка WiFi в режиме точки доступа
 * 
 * @param ssid SSID точки доступа
 * @param password Пароль точки доступа
 * @return true при успешной настройке, false в противном случае
 */
bool wifi_setup_ap(const char *ssid, const char *password);

/**
 * @brief Подключение к WiFi сети
 * 
 * @param ssid SSID сети
 * @param password Пароль сети
 * @return true при успешном подключении, false в противном случае
 */
bool wifi_connect_station(const char *ssid, const char *password);

/**
 * @brief Получение статуса подключения
 * 
 * @return Текущий статус подключения
 */
wifi_status_t wifi_get_status(void);

/**
 * @brief Получение IP адреса
 * 
 * @param ip_str Буфер для строки с IP адресом (минимум 16 символов)
 * @return true если IP адрес получен, false в противном случае
 */
bool wifi_get_ip_address(char *ip_str);

/**
 * @brief Отключение от WiFi сети
 */
void wifi_disconnect(void);

/**
 * @brief Изменение конфигурации WiFi
 * 
 * @param config Новая конфигурация
 * @return true при успешном изменении, false в противном случае
 */
bool wifi_reconfigure(const wifi_config_t *config);

#endif // WIFI_MANAGER_H

