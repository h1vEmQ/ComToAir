/**
 * @file web_server.h
 * @brief Веб-сервер для доступа к данным
 */

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Инициализация и запуск веб-сервера
 * 
 * @param port Порт для веб-сервера (по умолчанию 80)
 * @return true при успешном запуске, false в противном случае
 */
bool web_server_start(uint16_t port);

/**
 * @brief Остановка веб-сервера
 */
void web_server_stop(void);

/**
 * @brief Получение последних данных из буфера RS-232
 * 
 * @param buffer Буфер для данных
 * @param length Размер буфера
 * @return Количество байт в буфере
 */
size_t web_server_get_data(uint8_t *buffer, size_t length);

/**
 * @brief Установка данных для отправки клиентам
 * 
 * @param data Данные
 * @param length Длина данных
 */
void web_server_set_data(const uint8_t *data, size_t length);

/**
 * @brief Проверка статуса веб-сервера
 * 
 * @return true если сервер запущен, false в противном случае
 */
bool web_server_is_running(void);

#endif // WEB_SERVER_H

