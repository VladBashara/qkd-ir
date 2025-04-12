#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#ifndef CMAKE_BINARY_DIR
#define CMAKE_BINARY_DIR ""
#endif

#include "ldpc-utils.hpp"
#include "genetic_algo_utils.hpp"
#include "genetic_algo.hpp"
#include "file-processor.h"

#include <doctest/doctest.h>
#include <Eigen/Sparse>
#include <random>
#include <algorithm>
#include <utility>
#include <sstream>

TEST_SUITE_BEGIN("Validation");

TEST_CASE("popul_size validate") {
    double P_m = 0.4;
    std::string mat_path = CMAKE_BINARY_DIR + std::string("/experiments/data/BG1.alist");
    size_t Z = 3;
    std::pair<double, double> QBER_range = {0.0, 0.02};
    double QBER_step = 0.002;
    size_t mu = 6;
    size_t iter_amount = 12;
    int mutation_seed = 134;

    size_t popul_size = 1;

    CHECK_THROWS_WITH(
        genetic_algo(popul_size, P_m, mat_path, Z, QBER_range, QBER_step, mu, iter_amount),
        ( "popul_size <= 2 : " + std::to_string(popul_size) ).data()
    );
}

TEST_CASE("P_m validate") {
    size_t popul_size = 5;
    std::string mat_path = CMAKE_BINARY_DIR + std::string("/experiments/data/BG1.alist");
    size_t Z = 3;
    std::pair<double, double> QBER_range = {0.0, 0.02};
    double QBER_step = 0.002;
    size_t mu = 6;
    size_t iter_amount = 12;
    int mutation_seed = 134;

    double P_m = 4;
    
    CHECK_THROWS_WITH(
        genetic_algo(popul_size, P_m, mat_path, Z, QBER_range, QBER_step, mu, iter_amount),
        ( "P_m < 0 or P_m > 1 : " + std::to_string(4.000000) ).data()
    );
}

TEST_CASE("iter_amount validate") {
    size_t popul_size = 5;
    double P_m = 0.4;
    std::string mat_path = CMAKE_BINARY_DIR + std::string("/experiments/data/BG1.alist");
    size_t Z = 3;
    std::pair<double, double> QBER_range = {0.0, 0.02};
    double QBER_step = 0.002;
    size_t mu = 6;
    int mutation_seed = 134;

    size_t iter_amount = 0;

    CHECK_THROWS_WITH(
        genetic_algo(popul_size, P_m, mat_path, Z, QBER_range, QBER_step, mu, iter_amount),
        ( "iter_amount == " + std::to_string(iter_amount) ).data()
    );
}

TEST_CASE("invalid matrix name") {
    size_t popul_size = 5;
    double P_m = 0.4;
    size_t Z = 3;
    std::pair<double, double> QBER_range = {0.0, 0.02};
    double QBER_step = 0.002;
    size_t mu = 6;
    size_t iter_amount = 12;
    int mutation_seed = 134;

    std::string mat_path = CMAKE_BINARY_DIR + std::string("/experiments/data/somename.alist");

    CHECK_THROWS_WITH(
        genetic_algo(popul_size, P_m, mat_path, Z, QBER_range, QBER_step, mu, iter_amount),
        ("read_alist: file \""+mat_path+"\" not found!").data()
    );
}

TEST_SUITE_END();