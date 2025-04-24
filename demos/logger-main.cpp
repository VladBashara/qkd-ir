#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <random>
#include "LogSender.h"

template<typename CSV>
void generateRandomLogs(LogSender& sender, CSV& csv, int count, const std::string& source)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrInt(1, 1000);

    for (int i = 0; i < count; ++i)
    {
        std::string timestamp = getCurrentTimeStr();
        std::string severity = (distrInt(gen) % 2 == 0) ? "INFO" : "ALERT";
        std::string messageType = (source == "laser_driver") ? "setting_change" :
                                  (source == "integrity_control") ? "process_state" :
                                  (source == "sensor_module") ? "sensor_alert" : "network_event";
        std::string message = (source == "laser_driver") ? "TEC was turned on" :
                              (source == "integrity_control") ? "Process (victim)(12342) was changed" :
                              (source == "sensor_module") ? "Temperature exceeded threshold" :
                              "Unexpected network packet detected";

        std::string logMessage = "[" + timestamp + "] " + severity + " " + source + "," + messageType + ": " + message;

        sender << std::make_pair(LogLevel::INFO, logMessage);

        csv.writeRow(timestamp, severity, source, messageType, message);

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int main()
{
    LogSender logger1;
    LogSender logger2;

    logger1.addConsoleReceiver(LogLevel::INFO);
    logger1.addFileReceiver(LogLevel::INFO, "logs1.txt");
    logger1.addNetworkReceiver(LogLevel::INFO, "127.0.0.1", 8002);

    logger2.addConsoleReceiver(LogLevel::INFO);
    logger2.addFileReceiver(LogLevel::INFO, "logs2.txt");
    logger2.addNetworkReceiver(LogLevel::INFO, "127.0.0.1", 8003);

    CSVReceiverT<std::string, std::string, std::string, std::string, std::string> csv1(
        "logs1.csv", {"timestamp", "level", "source", "type", "message"}
    );

    CSVReceiverT<std::string, std::string, std::string, std::string, std::string> csv2(
        "logs2.csv", {"timestamp", "level", "source", "type", "message"}
    );

    std::cout << "Starting log generation...\n";

    std::thread t1(generateRandomLogs<decltype(csv1)>, std::ref(logger1), std::ref(csv1), 5, "sensor_module");
    std::thread t2(generateRandomLogs<decltype(csv2)>, std::ref(logger2), std::ref(csv2), 5, "laser_driver");

    t1.join();
    t2.join();

    std::cout << "Log generation completed.\n";

    return 0;
}
