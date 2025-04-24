#include "LogSender.h"

//LogSender& LogSender::getInstance()
//{
//    static LogSender instance();  
//    return instance;
//}

int64_t getCurrentMilliseconds()
{
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
};

std::string getCurrentTimeStr()
{
    int64_t current_stamp = getCurrentMilliseconds();
    std::time_t timestamp = current_stamp / MILLISECONDS_RATIO;
    struct std::tm datetime = *std::localtime(&timestamp);
    char timestampPart[21];
    std::strftime(timestampPart, sizeof(timestampPart), "%Y-%m-%d %H:%M:%S.", &datetime);
    int64_t milliseconds = current_stamp % MILLISECONDS_RATIO;

    char milPart[4];
    std::sprintf(milPart, "%03ld", milliseconds);
    return std::string(timestampPart) + std::string(milPart);
};

template<typename T>
LogSender& LogSender::operator<<(std::pair<LogLevel, T> msg)
{
    std::lock_guard<std::mutex> lock(queueMutex);
    std::ostringstream oss;
    oss << msg.second;
    logQueue.push({msg.first, oss.str()});
    cv.notify_one();
    return *this;
}

template LogSender& LogSender::operator<< <std::string>(std::pair<LogLevel, std::string>);

LogSender::LogSender() : stopFlag(false)
{
    senderThread = std::thread(&LogSender::processQueue, this);
}

LogSender::~LogSender()
{
    stopFlag = true;
    cv.notify_one();
    if (senderThread.joinable())
    {
        senderThread.join();
    }
}

void LogSender::addConsoleReceiver(LogLevel level)
{
    receivers[level].push_back(std::make_shared<ConsoleReceiver>());
}

void LogSender::addFileReceiver(LogLevel level, const std::string& filename)
{
    receivers[level].push_back(std::make_shared<FileReceiver>(filename));
}

void LogSender::addNetworkReceiver(LogLevel level, const std::string& ip, int port)
{
    receivers[level].push_back(std::make_shared<NetworkReceiver>(ip, port));
}

template<typename... Ts>
void LogSender::addCSVReceiver(LogLevel level, const std::string& filename, const std::vector<std::string>& columns)
{
	auto receiver = std::make_shared<CSVReceiverT<Ts...>>(filename, columns);
    csvReceivers.emplace_back(level, receiver);
}

void LogSender::processQueue()
{
    while (!stopFlag)
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        cv.wait(lock, [this] { return !logQueue.empty() || stopFlag; });

        while (!logQueue.empty())
        {
            auto logMessage = logQueue.front();
            logQueue.pop();
            lock.unlock();

            auto it = receivers.find(logMessage.first);
            if (it != receivers.end())
            {
                for (auto& receiver : it->second)
                {
                    (*receiver) << logMessage.second;
                }
            }

            lock.lock();
        }
    }
}
