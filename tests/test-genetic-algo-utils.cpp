#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#ifndef CMAKE_BINARY_DIR
#define CMAKE_BINARY_DIR ""
#endif

#include "ldpc-utils.hpp"
#include "genetic_algo_utils.hpp"
// #include "alist_matrix.h"
#include "file-processor.h"

#include <doctest/doctest.h>
#include <Eigen/Sparse>
#include <random>
#include <algorithm>


TEST_SUITE_BEGIN("Matrix crossover");

TEST_CASE("small matrices: success") {
    MyMatrix mat_a = vec_to_sparse_m({{1, 0, 1},
                                      {1, 1, 0},
                                      {0, 0, 1},});

    MyMatrix mat_b = vec_to_sparse_m({{0, 1, 1},
                                      {1, 0, 0},
                                      {1, 0, 1},});

    auto [mat_actual_1, mat_actual_2] = crossover(mat_a, mat_b);

    MyMatrix mat_expected_1 = vec_to_sparse_m({{1, 0, 1},
                                               {1, 0, 0},
                                               {1, 0, 1},});

    MyMatrix mat_expected_2 = vec_to_sparse_m({{0, 1, 1},
                                               {1, 1, 0},
                                               {0, 0, 1},});

    CHECK( mat_actual_1 == mat_expected_1 );
    CHECK( mat_actual_2 == mat_expected_2 );
}


TEST_CASE("small matrices: failure") {
    MyMatrix mat_a = vec_to_sparse_m({{1, 1, 1},
                                      {1, 1, 1},
                                      {0, 0, 1},});

    MyMatrix mat_b = vec_to_sparse_m({{0, 1, 1},
                                      {1, 0, 0},
                                      {1, 0, 1},});

    auto [mat_actual_1, mat_actual_2] = crossover(mat_a, mat_b);

    MyMatrix mat_expected_1 = vec_to_sparse_m({{1, 0, 1},
                                               {1, 0, 0},
                                               {1, 0, 1},});

    MyMatrix mat_expected_2 = vec_to_sparse_m({{0, 1, 1},
                                               {1, 1, 0},
                                               {0, 0, 1},});

    CHECK( mat_actual_1 != mat_expected_1 );
    CHECK( mat_actual_2 != mat_expected_2 );
}


TEST_CASE("small matrices: 1x1") {
    MyMatrix mat_a = vec_to_sparse_m({{1},});

    MyMatrix mat_b = vec_to_sparse_m({{0},});

    try {
        auto [mat_actual_1, mat_actual_2] = crossover(mat_a, mat_b);
    } catch (const std::exception &err) {
        CHECK( std::string(err.what()) == "rows < 2 or cols < 2" );
    }
}


TEST_CASE("small matrices: empty") {
    MyMatrix mat_a = vec_to_sparse_m({{},});

    MyMatrix mat_b = vec_to_sparse_m({{},});

    try {
        auto [mat_actual_1, mat_actual_2] = crossover(mat_a, mat_b);
    } catch (const std::exception &err) {
        CHECK( std::string(err.what()) == "rows < 2 or cols < 2" );
    }
}


TEST_CASE("small matrices: different shapes") {
    MyMatrix mat_a = vec_to_sparse_m({{1, 0, 1},
                                      {1, 1, 0},
                                      {0, 0, 1},});

    MyMatrix mat_b = vec_to_sparse_m({{0, 1},
                                      {1, 0},});

    try {
        auto [mat_actual_1, mat_actual_2] = crossover(mat_a, mat_b);
    } catch (const std::exception &err) {
        CHECK( std::string(err.what()) == "mat_a mat_b got different shapes" );
    }
}

TEST_SUITE_END();


TEST_SUITE_BEGIN("Matrix mutation");

TEST_CASE("invalid BG_type") {
    MyMatrix mat_in;

    MyMatrix mat;

    std::vector<std::pair<uint, uint>> pos_vec = {{1,2},};

    try {
        MyMatrix mat_actual = make_mutation(BG_type(-1), pos_vec, mat_in);
    } catch (const std::exception &err) {
        CHECK( std::string(err.what()) == "invalid BG_type in make_mutation()" );
    }
}


TEST_CASE("small matrices") {
    
    MyMatrix mat_in = vec_to_sparse_m({{1, 0, 1},
                                    {1, 1, 0},
                                    {0, 0, 1},});

    std::vector<std::pair<uint, uint>> pos_vec = {{1,1}, {2,0}};

    MyMatrix mat_actual = make_mutation(BG_type::NOT_5G, pos_vec, mat_in);

    MyMatrix mat_expected = vec_to_sparse_m({{1, 0, 1},
                                             {1, 0, 0},
                                             {1, 0, 1},});

    CHECK( mat_actual == mat_expected );
}


TEST_CASE("BG1: invalid pos") {
    MyMatrix mat_in;

    std::vector<std::pair<uint, uint>> pos_vec = {{4,25}, {20,19}};

    MyMatrix mat_actual = make_mutation(BG_type::BG1, pos_vec, mat_in);

    MyMatrix mat_BG1 = load_matrix_from_alist(CMAKE_BINARY_DIR + std::string("/data/BG1.alist"));

    MyMatrix mat_diff(mat_BG1.rows(), mat_BG1.cols());
    for (auto pos : pos_vec) {
        mat_diff.coeffRef(pos.first, pos.second) = GF2(1);
    }

    MyMatrix mat_expected = mat_BG1 + mat_diff;

    CHECK( mat_actual == mat_expected );
}


TEST_CASE("BG1: invalid pos") {

    std::vector<std::pair<uint, uint>> pos_vec = {{1,1}, {2,0}};

    MyMatrix mat_actual;

    try {
        mat_actual = make_mutation(BG_type::BG1, pos_vec);
    } catch (std::exception &err) {
        CHECK(std::string(err.what()) == "invalid pos in pos_vec: 1 1");
    }
}

TEST_SUITE_END();


TEST_SUITE_BEGIN("Matrix random mutation");

TEST_CASE("invalid BG_type") {
    uint bits_count = 5;
    int mutation_seed = 7;

    try {
        MyMatrix mat_actual = make_random_mutation(BG_type(-1), bits_count, mutation_seed);
    } catch (const std::exception &err) {
        CHECK( std::string(err.what()) == "invalid BG_type in make_random_mutation()" );
    }
}


TEST_CASE("small matrices: 100 loops") {

    std::mt19937 random_engine;

    for (int i = 0; i < 100; i++) {
        MyMatrix mat_in = vec_to_sparse_m({{1, 0, 1},
                                           {1, 1, 0},
                                           {0, 0, 1},});

        int mutation_seed = 7+i;

        random_engine.seed(mutation_seed);
        std::uniform_int_distribution<> some_dist{0, mat_in.rows()*mat_in.cols()};

        uint bits_count = some_dist(random_engine);
        
        MyMatrix mat_actual = make_random_mutation(BG_type::NOT_5G, bits_count, mutation_seed, mat_in);

        MyMatrix mat_diff = mat_actual + mat_in;

        CHECK( mat_diff.cast<int>().sum() == bits_count );
    }

}


TEST_CASE("BG1") {

    MyMatrix mat_in = load_matrix_from_alist(CMAKE_BINARY_DIR + std::string("/data/BG1.alist"));

    int mutation_seed = 7;

    std::mt19937 random_engine;
    random_engine.seed(mutation_seed);
    std::uniform_int_distribution<> some_dist{0, mat_in.rows()*mat_in.cols()};

    uint bits_count = some_dist(random_engine);
    
    MyMatrix mat_actual = make_random_mutation(BG_type::BG1, bits_count, mutation_seed);

    MyMatrix mat_diff = mat_actual + mat_in;

    CHECK( mat_diff.cast<int>().sum() == bits_count );

}


TEST_CASE("BG2") {

    MyMatrix mat_in = load_matrix_from_alist(CMAKE_BINARY_DIR + std::string("/data/BG2.alist"));

    int mutation_seed = 7;

    std::mt19937 random_engine;
    random_engine.seed(mutation_seed);
    std::uniform_int_distribution<> some_dist{0, mat_in.rows()*mat_in.cols()};

    uint bits_count = some_dist(random_engine);
    
    MyMatrix mat_actual = make_random_mutation(BG_type::BG2, bits_count, mutation_seed);

    MyMatrix mat_diff = mat_actual + mat_in;

    CHECK( mat_diff.cast<int>().sum() == bits_count );

}


TEST_SUITE_END();