#ifndef DATETIME_H
#define DATETIME_H

#include <ctime>
#include <string>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <cstdint> 

const int64_t MILLISECONDS_RATIO = 1000;

class Datetime {
private:
    int64_t since_epoch;

public:
    static int64_t TIMEZONE_DIFF;
    static const int64_t MOSCOW_TIMEZONE_DIFF = 60 * 60 * 3;
    static int64_t getTimezoneDiff();
    static void updateTimezone();

    Datetime();
    explicit Datetime(int64_t _epoch);
    explicit Datetime(const std::string& time_str);

    [[nodiscard]] std::string toStr() const;
    int64_t getEpoch() const;
};

#endif // DATETIME_H
