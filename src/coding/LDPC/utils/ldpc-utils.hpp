#ifndef UTILS_H
#define UTILS_H


#include <Eigen/Core>
#include <Eigen/Sparse>
#include "GF2.hpp"


class LLR
{
public:
	LLR() : m_val{0.0}, m_sign{0} {}
	LLR(double val) : m_val{std::abs(val)}, m_sign{val < 0} {}
	LLR(GF2 sign, double val) : m_val{val}, m_sign{sign} {}
	GF2 alpha() const
	{
		return m_sign;
	}
	double beta() const
	{
		return m_val;
	}
	operator double() const
	{
		return m_sign ? -m_val : m_val;
	}
	LLR operator+(LLR const& right)
	{
		return LLR{static_cast<double>(*this) + static_cast<double>(right)};
	}
	LLR const& operator+=(LLR const& right)
	{
		LLR sum{static_cast<double>(*this) + static_cast<double>(right)};
		m_val = sum.m_val;
		m_sign = sum.m_sign;
		return *this;
	}
	LLR const& operator-=(LLR const& right)
	{
		LLR sum{static_cast<double>(*this) - static_cast<double>(right)};
		m_val = sum.m_val;
		m_sign = sum.m_sign;
		return *this;
	}
	bool operator!=(LLR const& right)
	{
		return m_sign != right.m_sign || m_val != right.m_val;
	}
	
private:
	double m_val;
	GF2 m_sign;
};


class MemoryManager
{
public:
	MemoryManager(Eigen::SparseMatrix<GF2, Eigen::RowMajor> const& H, size_t number_of_threads);
	LLR ** get_Ms() const { return Ms; }
	LLR ** get_Es() const { return Es; }
	int * get_inner_index_ptr() const { return inner_index_ptr; }
	int * get_outer_index_ptr() const {return outer_index_ptr; }
	Eigen::Index get_non_zeros() const { return non_zeros; }
	~MemoryManager();
private:
	size_t compute_memory_amount(size_t nnz, size_t number_of_threads);
	LLR ** Ms{nullptr};
	LLR ** Es{nullptr};
	int * inner_index_ptr{nullptr};
	int * outer_index_ptr{nullptr};
	Eigen::Index non_zeros{0};
	char * raw_data{nullptr};
};


enum class BG_type{BG1, BG2, NOT_5G};
enum class shift_randomness{RANDOM, NO_RANDOM, COMBINE};


Eigen::SparseMatrix<GF2, Eigen::RowMajor> vec_to_sparse_m(std::vector<std::vector<GF2>> const& vec_repr);

Eigen::SparseMatrix<GF2, Eigen::RowMajor> augment_with_identity(Eigen::SparseMatrix<GF2, Eigen::RowMajor> const& H);

Eigen::SparseMatrix<GF2, Eigen::RowMajor> augment_with_identity_opt(Eigen::SparseMatrix<GF2, Eigen::RowMajor> const& H);

std::vector<LLR> llrs_by_bits(std::vector<GF2> const& bits, double e, size_t padded_part = 0);

Eigen::SparseMatrix<GF2, Eigen::RowMajor> enhance_from_base(Eigen::SparseMatrix<GF2, Eigen::RowMajor> const& BG, size_t Z_c);

Eigen::SparseMatrix<GF2, Eigen::RowMajor> shift_eyes(Eigen::SparseMatrix<GF2, Eigen::RowMajor> const& H, size_t Z_c, BG_type t, shift_randomness rnd = shift_randomness::NO_RANDOM);

Eigen::SparseMatrix<GF2, Eigen::RowMajor> augmentWithIdentity(Eigen::SparseMatrix<GF2, Eigen::RowMajor> const& Hin);

int Z_c2iLS(size_t Z_c);

int compute_shift(int row_index, int column_index, BG_type t, size_t Z_c);


#endif
