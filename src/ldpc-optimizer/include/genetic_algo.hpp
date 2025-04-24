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

template<typename CSV>
void generateLogs(LogSender &sender, CSV& csv, std::string severity, std::string source, std::string messageType, std::string message);
std::multimap<Result, GeneticMatrix, std::greater<Result>> calculate_obj_func(
    const BG_type bg_type, const std::multimap<Result, GeneticMatrix, std::greater<Result>> &population_map, const std::pair<double, double> QBER_range,
    const size_t Z
    );
template<typename CSV>
std::multimap<size_t, std::multimap<IntersectionMetric, GeneticMatrix, std::greater<IntersectionMetric>>> genetic_algo(const size_t popul_size, const double P_m, const std::string mat_path, const size_t Z,
    const std::pair<double, double> QBER_range, const size_t mu, const size_t iter_amount, const LogSender &logger, const CSV& csv);
