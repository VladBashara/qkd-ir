#ifndef LOG_RECEIVER_H
#define LOG_RECEIVER_H

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <memory>
#include <vector>
#include <stdexcept>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>  
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#endif

/**
 * @brief Базовый класс для логирования.
 */
class LogReceiver
{
public:
    virtual ~LogReceiver() = default;
    virtual void operator<<(const std::string& msg) = 0;
};

/**
 * @brief Класс логирования в консоль.
 */
class ConsoleReceiver : public LogReceiver
{
public:
    void operator<<(const std::string& msg) override;
};

/**
 * @brief Класс логирования в файл.
 */
class FileReceiver : public LogReceiver
{
private:
    std::ofstream logFile;

public:
    FileReceiver(const std::string& filename);
    void operator<<(const std::string& msg) override;
    ~FileReceiver();
};

/**
 * @brief Класс логирования в сеть.
 */
class NetworkReceiver : public LogReceiver
{
private:
    int sock;
    sockaddr_in serv_addr;

public:
    NetworkReceiver(const std::string& ip, int port);
    void operator<<(const std::string& msg) override;
    ~NetworkReceiver();
};

/**
 * @brief Класс логирования в CSV-файл.
 */
template<typename... Ts>
class CSVReceiverT : public LogReceiver
{
private:
    std::ofstream csvFile;
    std::string filename;
    std::vector<std::string> columns;

    void verifyOrInitializeFile();

    std::string formatColumns(const std::vector<std::string>& cols);

public:
    CSVReceiverT(const std::string& filename, const std::vector<std::string>& columnNames);
    void writeRow(Ts... args);
    void operator<<(const std::string& msg) override {} 
    ~CSVReceiverT();
};

#endif // LOG_RECEIVER_H
