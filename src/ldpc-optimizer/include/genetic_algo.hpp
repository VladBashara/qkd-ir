#include "result-mt.h"

#include <string>
#include <map>


struct GeneticMatrix {
    MyMatrix matrix;
    std::string history;
    std::string matrix_id;
};

Result make_default_key(std::pair<double, double> QBER_range, double QBER_step);
void print_result_of_genetic_algo(const std::multimap<size_t, std::multimap<Result, GeneticMatrix, std::greater<Result>>> &obj_func_map);
std::multimap<Result, GeneticMatrix, std::greater<Result>> calculate_obj_func(
    const BG_type bg_type, const std::multimap<Result, GeneticMatrix, std::greater<Result>> &population_map, const std::pair<double, double> QBER_range,
    const double QBER_step, const size_t Z
    );
std::multimap<size_t, std::multimap<Result, GeneticMatrix, std::greater<Result>>> genetic_algo(const size_t popul_size, const double P_m, const std::string mat_path,
                                const size_t Z, const std::pair<double, double> QBER_range,
                                const double QBER_step, const size_t mu, const size_t iter_amount);