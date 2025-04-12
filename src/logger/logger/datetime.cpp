#include "datetime.h"
#include <cstring>
#include <iomanip>
#include <sstream>

int64_t Datetime::TIMEZONE_DIFF = Datetime::getTimezoneDiff();

Datetime::Datetime() : since_epoch(0) {
    // Инициализация текущим временем
    since_epoch = std::time(nullptr) * MILLISECONDS_RATIO;
}

Datetime::Datetime(int64_t _epoch) : since_epoch(_epoch) {
    int64_t seconds_diff = Datetime::MOSCOW_TIMEZONE_DIFF - Datetime::getTimezoneDiff();
    since_epoch += seconds_diff * MILLISECONDS_RATIO;
}

Datetime::Datetime(const std::string& time_str) {
    const std::string time_details = time_str.substr(0, 20);
    std::string milliseconds_str = time_str.substr(20);
    struct tm tm{};
    std::istringstream ss(time_details);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S.");

    since_epoch = int64_t(mktime(&tm) * MILLISECONDS_RATIO) + std::stoi(milliseconds_str);
}

std::string Datetime::toStr() const {
    std::time_t timestamp = since_epoch / MILLISECONDS_RATIO;
    struct std::tm datetime = *std::localtime(&timestamp);
    char timestampPart[21];
    std::strftime(timestampPart, sizeof(timestampPart), "%Y-%m-%d %H:%M:%S.", &datetime);
    int64_t milliseconds = since_epoch % MILLISECONDS_RATIO;

    char milPart[4];
    std::sprintf(milPart, "%03ld", milliseconds);
    std::string out = std::string(timestampPart) + std::string(milPart);
    return out;
}

int64_t Datetime::getEpoch() const {
    return since_epoch;
}

int64_t Datetime::getTimezoneDiff() {
    int64_t diff;
    std::time_t secs = std::time(nullptr);
    std::tm timeParts{};
    std::memset(&timeParts, 0, sizeof(timeParts));
    std::tm timeInfo{};

#ifdef _WIN32
    localtime_s(&timeInfo, &secs);
#else
    localtime_r(&secs, &timeInfo);
#endif
    diff = std::mktime(&timeInfo);

    std::memset(&timeParts, 0, sizeof(timeParts));
#ifdef _WIN32
    gmtime_s(&timeInfo, &secs);
#else
    gmtime_r(&secs, &timeInfo);
#endif
    diff -= std::mktime(&timeInfo);
    return diff;
}


void Datetime::updateTimezone() {
    Datetime::TIMEZONE_DIFF = Datetime::getTimezoneDiff();
}
