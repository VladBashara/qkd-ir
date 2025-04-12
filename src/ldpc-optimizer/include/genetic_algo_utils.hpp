#ifndef GENETIC_ALGO_UTILS_H
#define GENETIC_ALGO_UTILS_H

#include "GF2.hpp"
#include "ldpc-utils.hpp"

#include <Eigen/Sparse>


using MyMatrix = Eigen::SparseMatrix<GF2, Eigen::RowMajor>;
using uint = unsigned int;


bool operator==(MyMatrix mat_a, MyMatrix mat_b);

bool is_valid_pos(BG_type type, std::pair<uint, uint> pos);

bool is_valid_pos(BG_type type, std::vector<std::pair<uint, uint>> pos_vec);

std::pair<MyMatrix, MyMatrix> crossover(const MyMatrix &mat_a, const MyMatrix &mat_b);

MyMatrix make_mutation(BG_type type, std::vector<std::pair<uint, uint>> pos_vec, const MyMatrix &mat_in=MyMatrix());

MyMatrix make_random_mutation(BG_type type, uint bits_count, int mutation_seed, const MyMatrix &mat_in=MyMatrix());

#endif