#ifndef TRIPLET_TASK_H
#define TRIPLET_TASK_H


#include "decoders.h"
#include <boost/serialization/access.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/array.hpp>
#include <iostream>
#include <tuple>


// template <typename Archive, typename... Types>
// void boost::serialization::serialize(Archive &ar, std::tuple<Types...> &t, const unsigned int)
// {
//	 std::apply([&](auto &...element)
//				 { ((ar & element), ...); },
//				 t);
// }


// namespace boost {
// namespace serialization {

// template<class Archive>
// void serialize(Archive & ar, GF2 & bit, const unsigned int version)
// {
// 	int i_bit{bit};
//	 ar & i_bit;
// }

// } // namespace serialization
// } // namespace boost


class Task {

private:
	bool work_completion = false;
	std::vector<std::pair<size_t, size_t>> one_positions;
	size_t H_rows;
	size_t H_cols;
	friend class boost::serialization::access;

	template<class Archive>
	void serialize(Archive & ar, const unsigned int version) {
		ar & this->ber_start;
		ar & this->ber_stop;
		ar & this->ber_step;
		ar & this->alg_type;
		ar & this->verbose;
		ar & this->one_positions;
		ar & this->work_completion;
		ar & this->H_rows;
		ar & this->H_cols;
	} 

public:
	Task(): work_completion{true} {}

	Task(double ber_start, double ber_stop, double ber_step, LDPC_algo alg_type,
					bool verbose, Eigen::SparseMatrix<GF2, Eigen::RowMajor> const& matrix) {

		this->ber_start = ber_start;
		this->ber_stop = ber_stop;
		this->ber_step = ber_step;
		this->alg_type = alg_type;
		this->verbose = verbose;
		this->H_rows = matrix.rows();
		this->H_cols = matrix.cols();

		for (int k=0; k < matrix.outerSize(); k++) {
			for (Eigen::SparseMatrix<GF2, Eigen::RowMajor>::InnerIterator it(matrix, k); it; ++it) {
				
				Eigen::Triplet<int> triplet{it.row(), it.col(), it.value()};
				this->one_positions.push_back({triplet.row(), triplet.col()});

			}
		}
	}

	~Task() = default;

	auto get_H() -> Eigen::SparseMatrix<GF2, Eigen::RowMajor> {
		Eigen::SparseMatrix<GF2, Eigen::RowMajor> matrix(H_rows, H_cols);

		std::vector<Eigen::Triplet<GF2>> triplets;
		triplets.reserve(one_positions.size());
		for (auto const& one_pos : one_positions) {
			triplets.push_back({one_pos.first, one_pos.second, 1});
		}

		matrix.setFromTriplets(triplets.begin(), triplets.end());
		
		return matrix;
	}

	auto make_work_complete() -> void {
		this->work_completion = true;
	}

	auto is_work_complete() -> bool {
		return this->work_completion;
	}

	double ber_start;
	double ber_stop;
	double ber_step;
	LDPC_algo alg_type;
	bool verbose;
};


#endif