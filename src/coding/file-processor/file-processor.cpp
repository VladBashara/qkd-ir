//
// Created by Vladimir Morozov on 01.06.2023.
//

#include "file-processor.h"

#include <chrono>

void dump_matrix(Eigen::SparseMatrix<GF2> const& m, std::string const& filename)
{
	Eigen::Matrix<GF2, Eigen::Dynamic, Eigen::Dynamic> m_dense = m.toDense();
	std::string m_str = matrix_to_alist_string(m_dense);

	std::ofstream m_file{filename};
	m_file << m_str;
	m_file.close();
}

void dump_current_matrix(Eigen::SparseMatrix<GF2> const& m, size_t opt_rows, size_t opt_cols)
{
	std::stringstream filename{"interm_BG1_"};
	filename << opt_rows << "_" << opt_cols << "_";
	filename << std::chrono::system_clock::now().time_since_epoch().count();
	filename << ".alist";
	dump_matrix(m.block<>(0, 0, opt_rows, opt_cols), filename.str());
}

// void draw_matrix(Eigen::SparseMatrix<GF2> const& m, std::string const& filename)
// {
// 	std::vector<std::vector<GF2>> temp_v(m.rows(), std::vector<GF2>(m.cols()));

// 	for (size_t i{0}; i < m.rows(); ++i)
// 		for (size_t j{0}; j < m.cols(); ++j)
// 			temp_v[i][j] = m.coeff(i, j);

// 	matplot::image(temp_v);

// 	matplot::save(filename, "pdf");
// }

Eigen::SparseMatrix<GF2> load_matrix_from_alist(std::string const& filename)
{
	alist_matrix matrix_alist;
	read_alist(filename, matrix_alist);
	return alist_to_matrix(matrix_alist).sparseView();
}
