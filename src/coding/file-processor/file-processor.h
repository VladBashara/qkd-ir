//
// Created by Vladimir Morozov on 01.06.2023.
//

#ifndef FILE_PROCESSOR_H
#define FILE_PROCESSOR_H

#include "alist_matrix.h"


void dump_matrix(Eigen::SparseMatrix<GF2> const& m, std::string const& filename);
void dump_current_matrix(Eigen::SparseMatrix<GF2> const& m, size_t opt_rows, size_t opt_cols);
// void draw_matrix(Eigen::SparseMatrix<GF2> const& m, std::string const& filename);
Eigen::SparseMatrix<GF2> load_matrix_from_alist(std::string const& filename);

#endif //FILE_PROCESSOR_H
