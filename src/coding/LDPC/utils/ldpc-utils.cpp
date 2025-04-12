#include "ldpc-utils.hpp"
#include "5g_bg_shifts.h"
#include <random>


MemoryManager::MemoryManager(Eigen::SparseMatrix<GF2, Eigen::RowMajor> const& H, size_t number_of_threads)
{
	non_zeros = H.nonZeros();

	size_t memory_size{0};
	memory_size = compute_memory_amount(non_zeros, number_of_threads);
	raw_data = new char[memory_size];

	Ms = reinterpret_cast<LLR**>(raw_data);

	size_t Es_shift = sizeof(LLR*) * number_of_threads;
	Es = reinterpret_cast<LLR**>(raw_data + Es_shift);

	size_t M_data_shift = Es_shift + sizeof(LLR*) * number_of_threads; //  + sizeof(LLR) * number_of_threads * nnz

 	size_t M_data_chunk_shift{0};
	for (size_t i{0}; i < number_of_threads; ++i) {
		Ms[i] = reinterpret_cast<LLR*>(raw_data + M_data_shift + i * sizeof(LLR) * non_zeros);
	}

	size_t E_data_shift = M_data_shift + sizeof(LLR) * number_of_threads * non_zeros;

 	size_t E_data_chunk_shift{0};
	for (size_t i{0}; i < number_of_threads; ++i) {
		Es[i] = reinterpret_cast<LLR*>(raw_data + E_data_shift + i * sizeof(LLR) * non_zeros);
	}

	inner_index_ptr = const_cast<int *>(H.innerIndexPtr());
	outer_index_ptr = const_cast<int *>(H.outerIndexPtr());
}


MemoryManager::~MemoryManager()
{
	delete[] raw_data;
}


size_t MemoryManager::compute_memory_amount(size_t nnz, size_t number_of_threads)
{
	return 2 * (sizeof(LLR *) * number_of_threads + sizeof(LLR) * number_of_threads * nnz);
}


Eigen::SparseMatrix<GF2, Eigen::RowMajor> vec_to_sparse_m(std::vector<std::vector<GF2>> const& vec_repr)
{
	std::vector<Eigen::Triplet<GF2>> triplets;
	triplets.reserve(vec_repr.size() * vec_repr[0].size());
	for (size_t i{0}; i < vec_repr.size(); ++i) {
		for (size_t j{0}; j < vec_repr[0].size(); ++j) {
			if (vec_repr[i][j] != 0) {
				triplets.push_back({i, j, vec_repr[i][j]});
			}
		}
	}
	Eigen::SparseMatrix<GF2, Eigen::RowMajor> res{vec_repr.size(), vec_repr[0].size()};
	res.setFromTriplets(triplets.begin(), triplets.end());
	res.makeCompressed();

	return res;
}


Eigen::SparseMatrix<GF2, Eigen::RowMajor> augment_with_identity(Eigen::SparseMatrix<GF2, Eigen::RowMajor> const& H)
{
	int hrows = (int) H.rows();
	int hcols = (int) H.cols();

	Eigen::SparseMatrix<GF2, Eigen::RowMajor> I(hrows,hrows);
	I.setIdentity();
	Eigen::SparseMatrix<GF2, Eigen::RowMajor> M(hcols + hrows, hrows);
	Eigen::SparseMatrix<GF2, Eigen::RowMajor> H_copy;
	H_copy = H.transpose();
	M.middleRows(0, hcols) = H_copy;
	M.middleRows(hcols, hrows) = I;

	Eigen::SparseMatrix<GF2, Eigen::RowMajor> H_out;

	H_out = M.transpose();
	H_out.makeCompressed();

	return H_out;
}


std::vector<LLR> llrs_by_bits(std::vector<GF2> const& bits, double e, size_t padded_part)
{
	std::vector<LLR> llrs;
	llrs.reserve(bits.size());

	double base_llr{log(1. - e) / e};
	double large_llr{1000.};

	for (size_t i{0}; i < bits.size() - padded_part; ++i) {
		llrs.push_back(bits[i] ? base_llr : -base_llr);
	}

	for (size_t i{bits.size() - padded_part}; i < bits.size(); ++i) {
		llrs.push_back(bits[i] ? -large_llr : large_llr);
	}

	return llrs;
}


std::mt19937 random_engine;

Eigen::SparseMatrix<GF2, Eigen::RowMajor> enhance_from_base(Eigen::SparseMatrix<GF2, Eigen::RowMajor> const& BG, size_t Z_c)
{
	Eigen::SparseMatrix<GF2> H(BG.rows() * Z_c, BG.cols() * Z_c);
	for (size_t bg_i{0}; bg_i < BG.rows(); ++bg_i) {
		for (size_t bg_j{0}; bg_j < BG.cols(); ++bg_j) {
			if (BG.coeff(bg_i, bg_j)) {
				for (size_t h_i{bg_i * Z_c}, h_j{bg_j * Z_c}; (h_i <= Z_c * (bg_i + 1) - 1) && (h_j <= Z_c * (bg_j + 1) - 1); ++h_i, ++h_j) {
					H.coeffRef(h_i, h_j) = 1;
				}
			}
		}
	}
	H.makeCompressed();
	return H;
}


Eigen::SparseMatrix<GF2, Eigen::RowMajor> shift_eyes(Eigen::SparseMatrix<GF2, Eigen::RowMajor> const& H, size_t Z_c, BG_type t, shift_randomness rnd)
{
	// Если размеры матрицы не кратны Z_c
	if (H.cols() % Z_c || H.rows() % Z_c)
		throw std::range_error{"H matrix size and Z_c value incompatible"};

	std::uniform_int_distribution<> shift_distribution(0, Z_c - 1);

	Eigen::Matrix<GF2, Eigen::Dynamic, Eigen::Dynamic> H_dense_copy = H;

	// Количество блоков единичных матриц в строках и столбцах
	size_t eyes_cols{H.cols() / Z_c};
	size_t eyes_rows{H.rows() / Z_c};

	for (size_t eye_row{0}; eye_row < eyes_rows; ++eye_row) {
		for (size_t eye_col{0}; eye_col < eyes_cols; ++eye_col) {
			Eigen::Matrix<GF2, Eigen::Dynamic, Eigen::Dynamic> eye_block = H_dense_copy.block(eye_row * Z_c, eye_col * Z_c, Z_c, Z_c);
			
			if (!eye_block.isZero()) {
				int shift{0};
				switch (rnd) {
					case shift_randomness::RANDOM:
					{
						shift = shift_distribution(random_engine);
						break;
					}
					case shift_randomness::NO_RANDOM:
					{
						shift = compute_shift(eye_row, eye_col, t, Z_c);
						break;
					}
					case shift_randomness::COMBINE:
					{
						try {
							shift = compute_shift(eye_row, eye_col, t, Z_c);
						}
						catch (std::out_of_range exc) {
							shift = shift_distribution(random_engine);
						}
						break;
					}
				}
				for (size_t i{0}; i < Z_c; ++i) {
					std::rotate(eye_block.row(i).begin(), eye_block.row(i).begin() + shift, eye_block.row(i).end());
				}
				H_dense_copy.block(eye_row * Z_c, eye_col * Z_c, Z_c, Z_c) = eye_block;
			}
		}
	}

	return H_dense_copy.sparseView();
}


Eigen::SparseMatrix<GF2, Eigen::RowMajor> augmentWithIdentity(Eigen::SparseMatrix<GF2, Eigen::RowMajor> const& Hin)
{
	int hrows = (int) Hin.rows();
	int hcols = (int) Hin.cols();

	Eigen::SparseMatrix<GF2, Eigen::RowMajor> I(hrows,hrows);
	I.setIdentity();
	Eigen::SparseMatrix<GF2, Eigen::RowMajor> M(hcols + hrows, hrows);
	Eigen::SparseMatrix<GF2, Eigen::RowMajor> Hin_copy;
	Hin_copy = Hin.transpose();
	M.middleRows(0, hcols) = Hin_copy;
	M.middleRows(hcols, hrows) = I;

	Eigen::SparseMatrix<GF2, Eigen::RowMajor> Hout;

	Hout = M.transpose();
	Hout.makeCompressed();

	return Hout;
}


int Z_c2iLS(size_t Z_c)
{
	for (std::pair<int, std::vector<int>> const& table_row : iLS_table) {
		if (std::find(table_row.second.begin(), table_row.second.end(), Z_c) != table_row.second.end()) {
			return table_row.first;
		}
	}
	throw std::runtime_error{"Invalid Z_c value"};
}


int compute_shift(int row_index, int column_index, BG_type t, size_t Z_c)
{
	shift_table_t shift_table;
	switch (t) {
		case BG_type::BG1:
			shift_table = BG1_shifts;
			break;
		case BG_type::BG2:
			shift_table = BG2_shifts;
			break;
		default:
			throw std::runtime_error{"Invalid matrix type"};
	}

	int iLS{Z_c2iLS(Z_c)};

	return shift_table[row_index].at(column_index).at(iLS) % Z_c;
}