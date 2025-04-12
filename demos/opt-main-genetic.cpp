#include "ldpc-utils.hpp"
#include "genetic_algo_utils.hpp"
#include "genetic_algo.hpp"
#include "file-processor.h"

#include <Eigen/Sparse>
#include <random>
#include <algorithm>
#include <utility>
#include <filesystem>
#include <chrono>

#ifndef CMAKE_BINARY_DIR
#define CMAKE_BINARY_DIR ""
#endif

void print_parameters(size_t popul_size, double P_m, std::string mat_path, size_t Z, std::pair<double, double> QBER_range, double QBER_step, size_t mu, size_t iter_amount) {
    std::cout << "Launch parameters: " << std::endl;
    std::cout << "size_t popul_size = " << popul_size << ";" << std::endl;
    std::cout << "double P_m = " << P_m << ";" << std::endl;
    size_t pos = std::string(CMAKE_BINARY_DIR).size();
    std::cout << "std::string mat_path = CMAKE_BINARY_DIR + std::string(\"" << mat_path.substr(pos) << "\");" << std::endl;
    std::cout << "size_t Z = " << Z << ";" << std::endl;
    std::cout << "std::pair<double, double> QBER_range = {" << QBER_range.first << ", " << QBER_range.second << "};" << std::endl;
    std::cout << "double QBER_step = " << QBER_step << ";" << std::endl;
    std::cout << "size_t mu = " << mu << ";" << std::endl;
    std::cout << "size_t iter_amount = " << iter_amount << ";" << std::endl;
    std::cout << std::string(7, '\n');
    std::cout.flush();
}

int main(int argc, char* argv[]) {
    std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();

    size_t popul_size = 3;
    double P_m = 0.3;
    std::string mat_path = CMAKE_BINARY_DIR + std::string("/experiments/data/BG1.alist"); // 46 rows / 68 cols = 0.32 code speed
    size_t Z = 3;
    std::pair<double, double> QBER_range = {0.0, 0.2};
    double QBER_step = 0.09;
    size_t mu = 7;
    size_t iter_amount = 2;

    print_parameters(popul_size, P_m, mat_path, Z, QBER_range, QBER_step, mu, iter_amount);

    std::multimap<size_t, std::multimap<Result, GeneticMatrix, std::greater<Result>>> some_res = genetic_algo(popul_size, P_m, mat_path, Z, QBER_range, QBER_step, mu, iter_amount);


    // print metric of default matrix
    auto mat_stem = std::filesystem::path(mat_path).stem().string();
    Result origin_mat_res;
    for (auto res : some_res.begin()->second) {
        if (res.second.history == mat_stem) {
            origin_mat_res = some_res.begin()->second.begin()->first;
        }
    }
    
    std::cout << mat_stem << " metric " << origin_mat_res.intersection_metric << std::endl;
    std::cout << origin_mat_res << std::endl;


    Result best_res = some_res.begin()->second.begin()->first;
    MyMatrix best_matrix = some_res.begin()->second.begin()->second.matrix;
    for (auto popul = some_res.begin(); popul != some_res.end(); popul++) {
        
        if (best_res < popul->second.begin()->first) {
            best_res = popul->second.begin()->first;
            best_matrix = popul->second.begin()->second.matrix;
        }

    }

    std::cout << "All epochs best metric: " << best_res.intersection_metric << std::endl;
    std::cout << best_res;

    // dump_matrix(best_matrix, CMAKE_BINARY_DIR + std::string("/experiments/data/dumped_best_matrix.alist"));

    std::chrono::steady_clock::time_point end_time = std::chrono::steady_clock::now();

    auto duration = end_time-start_time;
    auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
    duration -= hours;
    auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration);
    duration -= minutes;
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
    std::cout << std::string(2, '\n');
    std::cout << "Running time: " << hours.count() << "h " << minutes.count() << "m " << seconds.count() << "s" << std::endl;
    std::cout.flush();

    return 0;
}