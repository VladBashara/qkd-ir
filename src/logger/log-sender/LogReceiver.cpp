#include "LogReceiver.h"

void ConsoleReceiver::operator<<(const std::string& msg)
{
    std::cout << msg << std::endl;
}

FileReceiver::FileReceiver(const std::string& filename)
{
    logFile.open(filename, std::ios::app);
    if (!logFile)
    {
        throw std::runtime_error("Failed to open log file.");
    }
}

void FileReceiver::operator<<(const std::string& msg)
{
    logFile << msg << std::endl;
}

FileReceiver::~FileReceiver()
{
    if (logFile.is_open())
    {
        logFile.close();
    }
}

NetworkReceiver::NetworkReceiver(const std::string& ip, int port)
{
    #ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
    throw std::runtime_error("WSAStartup failed");
    }
    #endif

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        throw std::runtime_error("Socket creation error");
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    
    #ifdef _WIN32
    if (InetPtonA(AF_INET, ip.c_str(), &serv_addr.sin_addr) <= 0)
    #else
    if (inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr) <= 0)
    #endif
    {
        throw std::runtime_error("Invalid address/ Address not supported");
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        throw std::runtime_error("Connection Failed");
    }
}

void NetworkReceiver::operator<<(const std::string& msg)
{
    send(sock, msg.c_str(), msg.length(), 0);
}

NetworkReceiver::~NetworkReceiver()
{
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
    
#ifdef _WIN32
WSACleanup();
#endif
}

template<typename... Ts>
CSVReceiverT<Ts...>::CSVReceiverT(const std::string& filename, const std::vector<std::string>& columnNames)
    : filename(filename), columns(columnNames)
{
    if (columns.size() != sizeof...(Ts))
    {
        throw std::runtime_error("Column count does not match type count.");
    }

    verifyOrInitializeFile();
}

template<typename... Ts>
void CSVReceiverT<Ts...>::verifyOrInitializeFile()
{
    std::ifstream existingFile(filename);
    std::string firstLine;

    bool needHeader = true;

    if (existingFile.is_open() && std::getline(existingFile, firstLine))
    {
        std::istringstream iss(firstLine);
        std::vector<std::string> existingColumns;
        std::string col;

        while (std::getline(iss, col, ','))
        {
            existingColumns.push_back(col);
        }

        if (existingColumns == columns)
        {
            needHeader = false;
        }
        else
        {
            throw std::runtime_error("CSV column mismatch: expected " + formatColumns(columns) +
                                     ", found " + formatColumns(existingColumns));
        }
    }

    csvFile.open(filename, std::ios::app);
    if (!csvFile)
    {
        throw std::runtime_error("Failed to open CSV file.");
    }

    if (needHeader)
    {
        csvFile << formatColumns(columns) << "\n";
    }
}

template<typename... Ts>
std::string CSVReceiverT<Ts...>::formatColumns(const std::vector<std::string>& cols)
{
    std::ostringstream oss;
    for (size_t i = 0; i < cols.size(); ++i)
    {
        if (i > 0) oss << ",";
        oss << cols[i];
    }
    return oss.str();
}

template<typename... Ts>
void CSVReceiverT<Ts...>::writeRow(Ts... args)
{
    if (!csvFile.is_open()) return;

    std::ostringstream oss;
    ((oss << args << ","), ...);
    std::string row = oss.str();
    if (!row.empty())
        row.pop_back(); 

    csvFile << row << "\n";
}

template<typename... Ts>
CSVReceiverT<Ts...>::~CSVReceiverT()
{
    if (csvFile.is_open())
    {
        csvFile.close();
    }
}

template class CSVReceiverT<std::string, std::string, std::string, std::string, std::string>;
