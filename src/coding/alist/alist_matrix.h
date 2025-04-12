/**
 * @file alist_matrix.h
 * @author Yifan Zhang (scirocco_gti@yeah.net)
 * @brief
 * http://www.inference.org.uk/mackay/codes/alist.html
 * N: number of rows
 * M: number of cols
 * num_n: weight of a row
 * num_m: weight of a col
 * @date 2020-10-29 13:00:39
 * @modified: 2021-03-26 20:35:42
 */

#ifndef ALIST_MATRIX_H
#define ALIST_MATRIX_H

#include <string>
#include <sstream>
#include <vector>
#include <Eigen/Eigen>
#include <fstream>
#include "GF2.hpp"


// a struct to store spare matrix
struct alist_matrix {
    int nCols;
    int mRows;       /* size of the matrix */

    int nMaxColSum;
    int mMaxRowSum; /* actual biggest sizes */

    std::vector<int> nColSum; /* weight of each column n */
    std::vector<int> mRowSum; /* weight of each row, m */

    std::vector<int> mlist;    /* list of integer coordinates in the m direction where the non-zero entries are */
    std::vector<int> nlist;    /* list of integer coordinates in the n direction where the non-zero entries are */
};

void write_imatrix(std::ofstream &file,
                   std::vector<int> const &list,
                   int nrows,
                   int ncols);

void write_alist(std::string const &filename,
                 alist_matrix &a);

int read_alist(std::string const &filename,
               alist_matrix &a);

std::vector<int> read_ivector(std::ifstream &file,
                              const int length);

void write_ivector(std::ofstream &file,
                   std::vector<int> const &num_list,
                   const int length);

std::string matrix_to_alist_string(
        const Eigen::Matrix<GF2, Eigen::Dynamic, Eigen::Dynamic> &matrix);

// void matrix_to_alist(
//         Eigen::Matrix<GF2, Eigen::Dynamic, Eigen::Dynamic> &matrix,
//         alist_matrix &a);

Eigen::Matrix<GF2, Eigen::Dynamic, Eigen::Dynamic> alist_to_matrix(
        alist_matrix &data);

Eigen::SparseMatrix<GF2, Eigen::RowMajor> alist_to_sparse_matrix(alist_matrix &a_matrix);


#endif