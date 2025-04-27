#include "result-mt.h"
#include "LogSender.h"

#include <string>
#include <map>

using IntersectionMetric = double;

struct GeneticMatrix {
    MyMatrix matrix;
    std::string history;
    std::string matrix_id;
};

void generateLogs_console(LogSender &sender, std::string severity, std::string source, std::string messageType, std::string message);

// template<typename T>
void generateLogs_csv(CSVReceiverT<std::string, std::string, std::string, std::string, std::string> &csv, std::string severity, std::string source, std::string messageType, std::string message);

std::multimap<size_t, std::multimap<IntersectionMetric, GeneticMatrix, std::greater<IntersectionMetric>>> genetic_algo(const size_t popul_size, const double P_m, const std::string mat_path, const size_t Z,
    const std::pair<double, double> QBER_range, const size_t mu, const size_t iter_amount, LogSender &logger, CSVReceiverT<std::string, std::string, std::string, std::string, std::string> &csv);
