#ifndef CMAKE_BINARY_DIR
#define CMAKE_BINARY_DIR ""
#endif

#include "genetic_algo_utils.hpp"
#include "file-processor.h"
#include "alist_matrix.h"

#include <Eigen/Sparse>
#include <random>
#include <algorithm>


bool operator==(MyMatrix mat_a, MyMatrix mat_b) {

    if (mat_a.rows() != mat_b.rows() or mat_a.cols() != mat_b.cols()) { // входные матрицы имеют разные размерности
        return false;
    }

    int rows = mat_a.rows();
    int cols = mat_a.cols();

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (mat_a.coeff(i, j) != mat_b.coeff(i, j)) {
                return false;
            }
        }
    }

    return true;
}

bool is_valid_pos(BG_type type, std::pair<uint, uint> pos) { // TODO: add check matrix shape
    constexpr uint ROW_LIMIT = 4;
    constexpr uint COLUMN_LIMIT = 25;

    const uint row_index = pos.first;
    const uint col_index = pos.second;

    switch (type) {
    case BG_type::BG1 :
    case BG_type::BG2 :
        
        if (row_index < ROW_LIMIT or col_index > COLUMN_LIMIT) {
            return false;
        }

        break;
    
    case BG_type::NOT_5G :
        return true;
        break;

    default:
        return false;
    }
    
    return true;
}

bool is_valid_pos(BG_type type, std::vector<std::pair<uint, uint>> pos_vec) {

    for (auto pos : pos_vec) {
        
        if ( !is_valid_pos(type, pos) ) {
            return false;
        }

    }

    return true;
}

std::pair<MyMatrix, MyMatrix> crossover(const MyMatrix &mat_a, const MyMatrix &mat_b) {

    uint rows = 0;
    uint cols = 0;

    if (mat_a.rows() != mat_b.rows() or mat_a.cols() != mat_b.cols()) { // входные матрицы имеют разные размерности
        throw std::runtime_error("mat_a mat_b got different shapes");
    } else { // входные матрицы имеют одинаковую размерность
        rows = mat_a.rows();
        cols = mat_a.cols();
    }

    if (rows < 2 or cols < 2) {
        throw std::runtime_error("rows < 2 or cols < 2");
    }

    MyMatrix mat_out_1(rows, cols);
    MyMatrix mat_out_2(rows, cols);
    uint half_rows = rows / 2;

    mat_out_1.topRows(half_rows) = mat_a.topRows(half_rows);
    mat_out_1.bottomRows(rows - half_rows) = mat_b.bottomRows(rows - half_rows);

    mat_out_2.topRows(half_rows) = mat_b.topRows(half_rows);
    mat_out_2.bottomRows(rows - half_rows) = mat_a.bottomRows(rows - half_rows);
    
    return std::pair(mat_out_1, mat_out_2);
}

MyMatrix make_mutation(BG_type type, std::vector<std::pair<uint, uint>> pos_vec, const MyMatrix &mat_in) {
    
    is_valid_pos(type, pos_vec);

    MyMatrix mutated_matrix;
    alist_matrix mat_alist;
    int code = 0;

    switch (type) {
    case BG_type::BG1 :

        mutated_matrix = load_matrix_from_alist(CMAKE_BINARY_DIR + std::string("/data/BG1.alist"));

        break;
    
    case BG_type::BG2 :

        mutated_matrix = load_matrix_from_alist(CMAKE_BINARY_DIR + std::string("/data/BG2.alist"));

        break;
    
    case BG_type::NOT_5G :
        mutated_matrix = mat_in;
        break;

    default:
        throw std::runtime_error("invalid BG_type in make_mutation()");
    }

    for (std::pair<uint, uint> pos : pos_vec) {
        GF2 &elem = mutated_matrix.coeffRef(pos.first, pos.second);
        elem += GF2(1);
    }

    return mutated_matrix;
}

MyMatrix make_random_mutation(BG_type type, uint bits_count, int mutation_seed, const MyMatrix &mat_in) {
    std::mt19937 random_engine;
    random_engine.seed(mutation_seed);

    MyMatrix mat_loaded;

    switch (type) {
    case BG_type::BG1 :

        mat_loaded = load_matrix_from_alist(CMAKE_BINARY_DIR + std::string("/data/BG1.alist"));

        break;
    
    case BG_type::BG2 :

        mat_loaded = load_matrix_from_alist(CMAKE_BINARY_DIR + std::string("/data/BG2.alist"));

        break;
    
    case BG_type::NOT_5G :
        mat_loaded = mat_in;
        break;

    default:
        throw std::runtime_error("invalid BG_type in make_random_mutation()");
    }

    const int rows = (int)(mat_loaded.rows());
    const int cols = (int)(mat_loaded.cols());

    std::uniform_int_distribution<> y_dim_distribution{0, rows-1};
    std::uniform_int_distribution<> x_dim_distribution{0, cols-1};

    std::vector<std::pair<uint, uint>> pos_vec = {};
    while (pos_vec.size() != bits_count) {
        auto pos = std::pair<uint, uint>(y_dim_distribution(random_engine), x_dim_distribution(random_engine));

        if ( !is_valid_pos(type, pos) or std::find(pos_vec.begin(), pos_vec.end(), pos) != pos_vec.end()) {
            continue;
        }

        pos_vec.push_back(pos);
    }

    MyMatrix mutated_matrix = make_mutation(type, pos_vec, mat_loaded);

    return mutated_matrix;
}