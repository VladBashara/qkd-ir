#include "LogSender.h"

//LogSender& LogSender::getInstance()
//{
//    static LogSender instance();  
//    return instance;
//}

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

void LogSender::addCSVReceiver(LogLevel level, const std::string& filename, const std::vector<std::string>& columns)
{
    receivers[level].push_back(std::make_shared<CSVReceiver>(filename, columns));
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
