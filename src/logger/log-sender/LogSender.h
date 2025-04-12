/**
 * @defgroup LogSender LogSender
 * @brief Класс LogSender используется для отправки логов от генератора на логгер.
 * @{
*/

/**
 * @file
 * @brief Заголовочный файл с описанием класса LogSender.
*/

#ifndef LOGSENDER_H
#define LOGSENDER_H

#pragma once

#include <iostream>
#include <future>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <sstream>
#include <map>
#include <vector>
#include <memory>
#include "LogReceiver.h"

#ifdef ERROR
#undef ERROR
#endif

enum class LogLevel 
{
    INFO,
    ERROR,
    DATA,
    INFO_ERROR,
    INFO_DATA,
    ERROR_DATA,
    ALL
};

/**
 * @brief Класс LogSender создает очередь сообщений, которые отправляются на логгер.
 * Для работы с сокетами используется AF_INET и TCP-соединение.
*/

class LogSender
{
public:

/**
 * @brief Возвращает единственный экземпляр класса LogSender. 
 * @return LogSender& ссылка на экземпляр Logsender.
*/
    //static LogSender& getInstance();
    LogSender();
    ~LogSender();
/**
 * @brief Поддержка оператора << для добаления сообщений в очередь. 
 * @tparam T Тип сообщения.
 * @param msg Сообщение для логировавния.
 * @return LogSender& ссылка на текущий экземпляр LogSender.
*/    
    template<typename T>
    LogSender& operator<<(std::pair<LogLevel, T> msg);
    
    void addConsoleReceiver(LogLevel level);
    void addFileReceiver(LogLevel level, const std::string& filename);
    void addNetworkReceiver(LogLevel level, const std::string& ip, int port);
    void addCSVReceiver(LogLevel level, const std::string& filename, const std::vector<std::string>& columns);
    
/// Запрещает копирование и перемещение
	LogSender(const LogSender&) = delete;
	LogSender& operator=(const LogSender&) = delete;
	LogSender(LogSender&&) = delete;
	LogSender& operator=(LogSender&&) = delete;
	
private:
            
/**
 * @brief Обрабатывает очередь сообщений и отправляет их на логгер.
 * Ждет новые сообщения и отправляет их по мере поступления.
*/
    void processQueue();
    
    std::queue<std::pair<LogLevel, std::string>> logQueue; /**< Очередь сообщений для отправки */
    std::mutex queueMutex; /**< Мьютекс для защиты доступа к очереди */
    std::condition_variable cv; /**< Условная переменна для управления потоком отправки */
    std::thread senderThread; /**< Поток для асинхронной отправки сообщений */
    std::atomic<bool> stopFlag; /**< Флаг остановки потока отправки */
    
    std::map<LogLevel, std::vector<std::shared_ptr<LogReceiver>>> receivers;
};

#endif

/**
 * @}
*/
