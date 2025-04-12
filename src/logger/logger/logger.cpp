#include "logger.h"
#include "loggerDB.h"

Logger::Logger(int generatorPortMin, int generatorPortMax, int clientPort, const std::string &logFilename,
               const std::string &dbFilename)
        : filename(logFilename), running(true), generatorPortMin(generatorPortMin), generatorPortMax(generatorPortMax) 
{
    
    #ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) 
    {
        throw std::runtime_error("WSAStartup failed");
    }
    #endif

    int opt = 1;
    struct sockaddr_in address;

    // Создание сокетов для диапазона портов
    for (int port = generatorPortMin; port <= generatorPortMax; ++port) 
    {
        int sock;
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == 0)
            throw std::runtime_error("Socket creation error");

        #ifdef _WIN32
        if (setsockopt(sock, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, reinterpret_cast<const char*>(&opt), sizeof(opt)))
        #else
        if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt)))
        #endif
            throw std::runtime_error("Setsockopt error");

        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);

        if (bind(sock, (struct sockaddr *) &address, sizeof(address)) < 0) 
        {
            std::cerr << "Bind error on port " << port << std::endl;
            CLOSESOCKET(sock);
            continue;
        }

        if (listen(sock, 3) < 0)
            throw std::runtime_error("Listen error");

        generatorSockets.push_back(sock);
    }

    // Создание сокета для клиента
    if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
        throw std::runtime_error("Socket creation error");

    #ifdef _WIN32
    if (setsockopt(clientSocket, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, reinterpret_cast<const char*>(&opt), sizeof(opt)))
    #else
    if (setsockopt(clientSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt)))
    #endif
        throw std::runtime_error("Setsockopt error");

    address.sin_port = htons(clientPort);

    if (bind(clientSocket, (struct sockaddr *) &address, sizeof(address)) < 0) 
    {
        int error_code = errno; // https://man7.org/linux/man-pages/man3/errno.3.html#NOTES
        throw std::runtime_error("Bind error: " + std::string(strerror(error_code)));
    }


    if (listen(clientSocket, 3) < 0)
        throw std::runtime_error("Listen error");

    // Открытие подключения к SQLite
    if (sqlite3_open(dbFilename.c_str(), &db) != SQLITE_OK) {
        throw std::runtime_error("Error opening SQLite database: " + std::string(sqlite3_errmsg(db)));
    }

    // Создание таблиц
    createLaserSetTable();
    createTable();
}

void Logger::connectClient() 
{
    clientFuture = std::async(std::launch::async, [&]() 
    {
        while (running) 
        {
            int newSocket;
            struct sockaddr_in address;
            socklen_t addrlen = sizeof(address);

            if ((newSocket = accept(clientSocket, (struct sockaddr *) &address, &addrlen)) < 0)
                throw std::runtime_error("Accept error");
            std::thread(&Logger::handleClientConnection, this, newSocket).detach();
        }
    });
}

void Logger::connectGenerators() 
{
    generatorFuture = std::async(std::launch::async, [&]() 
    {
        acceptGenerators();
    });
}

void Logger::acceptGenerators() 
{
    fd_set readfds;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    while (running) 
    {
        FD_ZERO(&readfds);
        int max_sd = 0;

        for (int sock: generatorSockets) 
        {
            FD_SET(sock, &readfds);
            if (sock > max_sd) max_sd = sock;
        }

        timeval timeout = {1, 0};  // Тайм-аут 1 секунда
        int activity = select(max_sd + 1, &readfds, NULL, NULL, &timeout);
        if (activity < 0 && running)
            throw std::runtime_error("Select error");

        for (int sock: generatorSockets) 
        {
            if (FD_ISSET(sock, &readfds)) 
            {
                int newSocket = accept(sock, (struct sockaddr *) &address, &addrlen);
                if (newSocket >= 0)
                    std::thread(&Logger::handleGeneratorConnection, this, newSocket).detach();
            }
        }
    }
}


void Logger::handleClientConnection(int socket) 
{
    char buffer[1024];
    while (true) 
    {
        ssize_t bytesReceived = recv(socket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) 
        {
            CLOSESOCKET(socket);
            break;
        }
        buffer[bytesReceived] = '\0';
        std::string level(buffer);

        std::lock_guard<std::mutex> guard(logMutex);

        const char *selectSQL = R"(
            SELECT timestamp, level, message, turn_on, importance, baud_rate, filepath, current, current_max,
                   tec_temperature, tec_temperature_max, tec_temperature_min, tec_current_limit,
                   sensor_temperature_min, sensor_temperature_max, frequency, duration
            FROM laser_driver_setting WHERE level = ?;
        )";
        sqlite3_stmt *stmt;
        if (sqlite3_prepare_v2(db, selectSQL, -1, &stmt, nullptr) == SQLITE_OK) 
        {
            sqlite3_bind_text(stmt, 1, level.c_str(), -1, SQLITE_STATIC);

            while (sqlite3_step(stmt) == SQLITE_ROW) 
            {
                int64_t timestamp = sqlite3_column_int64(stmt, 0);
                std::string formattedTimestamp = Datetime(timestamp).toStr();

                std::string level = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
                std::string command = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
                int turn_on = sqlite3_column_int(stmt, 3);
                int importance = sqlite3_column_int(stmt, 4);
                int baud_rate = sqlite3_column_int(stmt, 5);
                std::string filepath = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 6));
                double current = sqlite3_column_double(stmt, 7);
                double current_max = sqlite3_column_double(stmt, 8);
                double tec_temperature = sqlite3_column_double(stmt, 9);
                double tec_temperature_max = sqlite3_column_double(stmt, 10);
                double tec_temperature_min = sqlite3_column_double(stmt, 11);
                double tec_current_limit = sqlite3_column_double(stmt, 12);
                double sensor_temperature_min = sqlite3_column_double(stmt, 13);
                double sensor_temperature_max = sqlite3_column_double(stmt, 14);
                double frequency = sqlite3_column_double(stmt, 15);
                double duration = sqlite3_column_double(stmt, 16);

                std::ostringstream oss;
                oss << "[" << formattedTimestamp << "] [" << level << "] " << command << ": "
                    << turn_on << ";" << importance << ";" << baud_rate << ";" << filepath << ";"
                    << current << ";" << current_max << ";" << tec_temperature << ";"
                    << tec_temperature_max << ";" << tec_temperature_min << ";" << tec_current_limit << ";"
                    << sensor_temperature_min << ";" << sensor_temperature_max << ";"
                    << frequency << ";" << duration;

                std::string logEntry = oss.str();
                if (send(socket, logEntry.c_str(), logEntry.size(), 0) == -1) 
                {
                    CLOSESOCKET(socket);
                    sqlite3_finalize(stmt);
                    throw std::runtime_error("Error sending log entry");
                }
            }
            sqlite3_finalize(stmt);
        } 
        else 
        {
            std::cerr << "Error preparing SQL statement: " << sqlite3_errmsg(db) << std::endl;
        }
    }
}

void Logger::handleGeneratorConnection(int socket) 
{
    char buffer[1024];
    //std::ofstream logFile(filename, std::ios::app);
    //if (!logFile) 
    //{
    //    throw std::runtime_error("Error opening log file: " + filename);
    //}

    // Получаем порт, с которого пришло соединение
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    getsockname(socket, (struct sockaddr *) &addr, &addr_len);
	
	std::cout << "Accepted connection from generator!" << std::endl;
	
    while (true) 
    {
   	 	//std::cout << "Waiting for logs..." << std::endl; 
        ssize_t bytesReceived = recv(socket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) 
        {
        	std::cout << "Generator disconnected." << std::endl;
            CLOSESOCKET(socket);         
            break;
        }
		
        buffer[bytesReceived] = '\0';
        std::string logEntry(buffer);
        
        std::cout << "Log from network: " << logEntry << std::endl;
		
        size_t timestampEnd = logEntry.find(']');
        size_t severityStart = logEntry.find(' ', timestampEnd + 1);
        size_t sourceStart = logEntry.find(' ', severityStart + 1);
        size_t messageTypeStart = logEntry.find(',', sourceStart + 1);
        size_t messageStart = logEntry.find(':', messageTypeStart + 1);
        if (timestampEnd == std::string::npos || severityStart == std::string::npos ||
            sourceStart == std::string::npos || messageTypeStart == std::string::npos ||
            messageStart == std::string::npos) {
            std::cerr << "Invalid log format: " << logEntry << std::endl;
            continue;
        }

        std::string timestampStr = logEntry.substr(1, timestampEnd - 1);
        std::string severity = logEntry.substr(severityStart + 1, sourceStart - severityStart - 1);
        std::string source = logEntry.substr(sourceStart + 1, messageTypeStart - sourceStart - 1);
        std::string messageType = logEntry.substr(messageTypeStart + 1, messageStart - messageTypeStart - 1);
        std::string message = logEntry.substr(messageStart + 2);
        //logFile << logEntry << std::endl;

        // Преобразуем время из сообщения в epoch
        Datetime datetime(timestampStr);
        int64_t timestamp = datetime.getEpoch();
        if (messageType == "laser_driver_setting") 
        {
            std::istringstream iss(message);
            std::string token;
            std::vector<std::string> paramList;
            while (std::getline(iss, token, ';')) 
            {
                paramList.push_back(token);
            }

            // Если параметров меньше 14, добавляем значения по умолчанию
            while (paramList.size() < 14) 
            {
                paramList.emplace_back("0");
            }

            // Вставка в таблицу laser_settings
            insertIntoLaserSetTable(timestamp, severity, source, paramList);
        } 
        else 
        {
            // Вставка в основную таблицу
            insertIntoTable(timestamp, severity, source, messageType, message);
        }

    }

    //logFile.close();
}

void Logger::stop() 
{
    running = false;
    generatorFuture.get();
    clientFuture.get();
}

Logger::~Logger() 
{
    stop();
    // Закрываем все сокеты генераторов
    for (int sock: generatorSockets) 
    {
        CLOSESOCKET(sock);
    }

    // Закрываем клиентский сокет
    CLOSESOCKET(clientSocket);

    #ifdef _WIN32
    WSACleanup();
    #endif
    
    sqlite3_close(db);
}
