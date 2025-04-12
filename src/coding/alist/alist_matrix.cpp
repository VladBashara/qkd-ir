#include "alist_matrix.h"
#include <Eigen/Eigen>
#include <iostream>
#include <iterator>


using namespace Eigen;


std::vector<int> read_ivector(std::ifstream& file, const int length)
{
	std::vector<int> num_list(length);
	for (size_t i{0}; i < length; ++i) {
		if (!(file >> num_list[i])) {
			std::cerr << "read_ivector: Reading failed!\n";
			return {};
		}
	}
	return num_list;
}

std::vector<int> read_imatrix(std::ifstream& file, const int nrows, const int ncols, const std::vector<int>& num_list)
{
	// initialize with zeros first
	std::vector<int> list(nrows * ncols, 0);
    for (size_t i{0}; i < nrows; ++i) {
        for (size_t j{0}; j < num_list[i]; ++j) {
			size_t index = i * ncols + j;
            if (!(file >> list[index])) {
				std::cerr << "read_imatrix: Reading failed!\n";
                return {};
            }
            // some alist contains 0
            if (!list[index]) {
                j--;
            }
        }
    }
    return list;
}

// read alist from file, return 0 if ok
int read_alist(std::string const& filename, alist_matrix &a)
{
    /* this assumes that mlist and nlist have the form of a rectangular
   matrix in the file; if lists have unequal lengths, then the
   entries should be present (eg zero values) but are ignored
   */

	std::ifstream file{filename};

	if (!file) {
		std::cerr << "read_alist: file \"" << filename << "\" not found!\n";
		return 1;
	}

	file >> a.nCols >> a.mRows;
	file >> a.nMaxColSum >> a.mMaxRowSum;
    
    if ((a.nColSum = read_ivector(file, a.nCols)).empty()) {return EOF;}
	if ((a.mRowSum = read_ivector(file, a.mRows)).empty()) {return EOF;}

    if ((a.nlist = read_imatrix(file, a.nCols, a.nMaxColSum, a.nColSum)).empty()) {return EOF;}
    if ((a.mlist = read_imatrix(file, a.mRows, a.mMaxRowSum,  a.mRowSum)).empty()) {return EOF;}

    return 0;
}

void write_ivector(std::ofstream& file, std::vector<int> const& num_list, const int length)
{
	std::ostream_iterator<int> fileIter(file, " ");
	std::copy(num_list.begin(), num_list.end(), fileIter);
	file << std::endl;
}

void write_imatrix(std::ofstream& file, std::vector<int> const& list, int nrows, int ncols)
{
    for (size_t i{0}; i < nrows; ++i) {
        for (size_t j{0}; j < ncols; ++j) {
            size_t index = i * ncols + j;
            file << list[index] << " ";
        }
		file << std::endl;
    }
}


void write_alist(std::string const& filename, alist_matrix &a)
{
	std::ofstream file{filename};
	if (!file) {
		std::cerr << "write_alist: file \"" << filename << "\" not found!\n";
	}

	file << a.nCols << " " << a.mRows << std::endl;
	file << a.nMaxColSum << " " << a.mMaxRowSum << std::endl;

    write_ivector(file, a.nColSum, a.nCols);
    write_ivector(file, a.mRowSum,  a.mRows);
    write_imatrix(file, a.nlist, a.nCols,  a.nMaxColSum);
    write_imatrix(file, a.mlist, a.mRows, a.mMaxRowSum);
}

std::string matrix_to_string(const Eigen::Matrix<int,Dynamic,Dynamic> &matrix) {
    int nRows = matrix.rows();
    int nCols = matrix.cols();

    std::stringstream oss;
    for (int i = 0; i < nRows; ++i){
        for (int j = 0; j < nCols; ++j){
            oss << matrix(i,j) << " ";
        }
        oss << "\n";
    }
    return oss.str();
}

std::string matrix_to_alist_string(const Matrix<GF2,Dynamic,Dynamic> &matrix) {
   
    int nRows = matrix.rows();
    int nCols = matrix.cols();

    Matrix<int,1,Dynamic> colSum = matrix.cast<int>().colwise().sum();
    Matrix<int,1,Dynamic> rowSum = matrix.cast<int>().rowwise().sum();
    
    int maxColSum = colSum.maxCoeff();
    int maxRowSum = rowSum.maxCoeff();
    
    // output to s
    std::ostringstream oss;
    oss << nCols << " " << nRows << "\n";
    oss << maxColSum << " " << maxRowSum << "\n";

    oss << matrix_to_string(colSum);
    oss << matrix_to_string(rowSum);

    int padZeros;

    // for every column
    for (int col = 0; col < nCols; ++col){
        padZeros = maxColSum;
        for (int row = 0; row < nRows; ++row){
            if (matrix(row,col) != 0){
                padZeros = padZeros - 1;
                oss << row + 1 << " ";
            }
        }
        if (padZeros > 0){
            Matrix<int,Dynamic,Dynamic> zeros = Matrix<int,Dynamic,Dynamic>::Zero(1,padZeros);
            oss << matrix_to_string(zeros);
        }
        else{
            oss << "\n";
        }
    }

    // for every row
    for (int row = 0; row < nRows; ++row){
        padZeros = maxRowSum;
        for (int col = 0; col < nCols; ++col){
            if (matrix(row,col) != 0){
                padZeros = padZeros - 1;
                oss << col + 1 << " ";
            }
        }

        if (padZeros > 0){
            oss << matrix_to_string(Eigen::Matrix<int,Dynamic,Dynamic>::Zero(1,padZeros));
        }
        else{
            oss << "\n";
        }
    }

    return oss.str();
}

// void matrix_to_alist(Matrix<GF2,Dynamic,Dynamic> &matrix, alist_matrix &a) {
//     std::string alist_str = matrix_to_alist_string(matrix);
// 	std::string tempFileName{std::filesystem::temp_directory_path().append("test_alist.tmp").string()};
// 	std::ofstream tmpf{tempFileName, std::ios::trunc};
// 	tmpf << alist_str;
// 	tmpf.close();
// 	read_alist(tempFileName, a);
// 	std::filesystem::remove(tempFileName);
// }


Matrix<GF2,Dynamic,Dynamic> alist_to_matrix(alist_matrix &a_matrix)
{
    int nCols = a_matrix.nCols;
    int mRows = a_matrix.mRows;
    Matrix<GF2,Dynamic,Dynamic> ret = Matrix<GF2,Dynamic,Dynamic>::Zero(mRows, nCols);

    int index;
    for (int i = 0; i < a_matrix.nCols; i++) {
        for (int j = 0; j < a_matrix.nColSum[i]; j++) {
            index = i * a_matrix.nMaxColSum + j;

            int ii = a_matrix.nlist[index] - 1;
            int jj = i;
            ret(ii,jj) = 1;
        }
    }
    return ret;
}

SparseMatrix<GF2, RowMajor> alist_to_sparse_matrix(alist_matrix &a_matrix) {
    // see: https://github.com/Sciroccogti/LDPC-with-SIMD/blob/master/src/Alist/Alist.cpp
    SparseMatrix<GF2, RowMajor> ret(a_matrix.mRows, a_matrix.nCols);
    int index;
    for (int i = 0; i < a_matrix.nCols; i++) {
        for (int j = 0; j < a_matrix.nColSum[i]; j++) {
            index = i * a_matrix.mRows + j;
            ret.insert(a_matrix.nlist[index] - 1, i) = 1;
        }
    }
    ret.makeCompressed();
    return ret;
}