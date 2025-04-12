/**
 * @defgroup logger Logger
 * @brief Класс Logger используется для приема логов от генератора и отправки их клиенту.
 * @{
*/

/**
 * @file
 * @brief Заголовочный файл с описанием класса Logger.
*/

#ifndef LOGGER_H
#define LOGGER_H

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#define CLOSESOCKET closesocket
#else
#include <arpa/inet.h>
#include <unistd.h>
#define CLOSESOCKET close
#endif

#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <string>
#include <limits>
#include <stdexcept>
#include <future>
#include <sqlite/sqlite3.h>
#include <sstream>
#include <iomanip>
#include <vector>
#include "datetime.h"

/**
 * @brief Класс Logger управляет подключениями от генераторов логов и клиентов.
 * Сохраняет логи в файл и базу данных, отправляет клиентам логи из базы данных.
 */
class Logger
{
public:
/**
 * @brief Конструктор класса Logger. Создает сокеты для подключения генератора и клиента.
 * @param generatorPort Порт для подключения генератора.
 * @param clientPort Порт для подключения клиента.
 * @param filename Имя файла, куда будут записываться логи.
 * @throws std::runtime_error если не удается создать сокет или установить соединение.
*/
    Logger(int generatorPortMin, int generatorPortMax, int clientPort, const std::string& logFilename, const std::string& dbFilename);
    
/**
 * @brief Подключение клиента. 
 * Запускает поток для ожидания подключения клиента и обработки его запросов.
*/
    void connectClient();
    
/**
 * @brief Подключение генератора.
 * Запускает поток для приема логов от генератора.
*/
    void connectGenerators(); 
    
    static std::string toStr(int64_t timestamp);
    
/**
 * @brief Завершает работу логгера.
*/
    void stop();
    
/**
 * @brief Деструктор класса Logger.
 *  Завершает потоки и закрывает все открытые сокеты.
*/    
    ~Logger();



private:

/**
 * @brief Обрабатывает подключение клиента.
 * Принимет запрос от клиента и отправляет логи из файла по указанному уровню.
 * @param socket Сокет, через который осуществляется связь с клиентом.
*/
    void handleClientConnection(int socket);
    
/**
 * @brief Обрабатывает подключение генератора.
 * Принимет сообщения от генератора и записывает их в файл логов.
 * @param socket Сокет, через который осуществляется связь с генератором.
*/
    void handleGeneratorConnection(int socket);
    
/**
 * @brief Принимает подключение от нового генератора.
 * Принимет и подключает генераторы.
*/
    void acceptGenerators(); 

    void insertIntoLaserSetTable(int64_t timestamp, const std::string& level, const std::string& command, const std::vector<std::string>& params);
    void insertIntoTable(int64_t timestamp, const std::string& severity, const std::string& source, const std::string& messageType, const std::string& message);
    
    int getOrInsertSourceId(const std::string& sourceName);
    int getOrInsertMessageTypeId(const std::string& messageTypeName);

    void createLaserSetTable(); // Создание таблицы лазера
    void createTable(); // Создание основной таблицы
    
    std::vector<int> generatorSockets;  /**< Сокет для подключения генератора */
    int clientSocket;                  /**< Сокет для подключения клиента */
    std::string filename;              /**< Имя файла для хранения логов */
    sqlite3* db;                       /**< Указатель на базу данных SQLite */
    std::unordered_map<std::string, size_t> lastSentLineMap; /**< Карта для отслеживания строк по уровням */
    std::mutex logMutex;               /**< Мьютекс для синхронизации доступа к логам */
    std::future<void> generatorFuture; /**< Объект для управления потоком генератора */
    std::future<void> clientFuture;    /**< Объект для управления потоком клиента */
    bool running;                      /**< Флаг работы логгера */
    int generatorPortMin, generatorPortMax; /**< Диапазон портов для генераторов */
};


#endif
