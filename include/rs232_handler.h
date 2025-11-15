/**
 * @file rs232_handler.h
 * @brief Обработчик интерфейса RS-232
 */

#ifndef RS232_HANDLER_H
#define RS232_HANDLER_H

#include "driver/uart.h"
#include "driver/gpio.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Структура конфигурации RS-232
 */
typedef struct {
    uint32_t baud_rate;        // Скорость передачи
    uart_word_length_t data_bits;  // Количество бит данных
    uart_parity_t parity;      // Четность
    uart_stop_bits_t stop_bits; // Стоп-биты
} rs232_config_t;

/**
 * @brief Инициализация UART для работы с RS-232
 * 
 * @param config Конфигурация RS-232
 * @return true при успешной инициализации, false в противном случае
 */
bool rs232_init(const rs232_config_t *config);

/**
 * @brief Чтение данных из UART
 * 
 * @param buffer Буфер для данных
 * @param length Размер буфера
 * @param timeout_ms Таймаут в миллисекундах
 * @return Количество прочитанных байт
 */
int rs232_read(uint8_t *buffer, size_t length, uint32_t timeout_ms);

/**
 * @brief Запись данных в UART
 * 
 * @param data Данные для отправки
 * @param length Длина данных
 * @return Количество записанных байт
 */
int rs232_write(const uint8_t *data, size_t length);

/**
 * @brief Изменение конфигурации RS-232
 * 
 * @param config Новая конфигурация
 * @return true при успешном изменении, false в противном случае
 */
bool rs232_reconfigure(const rs232_config_t *config);

/**
 * @brief Получение текущей конфигурации RS-232
 * 
 * @param config Указатель на структуру для сохранения конфигурации
 */
void rs232_get_config(rs232_config_t *config);

/**
 * @brief Очистка буферов UART
 */
void rs232_flush(void);

#endif // RS232_HANDLER_H

