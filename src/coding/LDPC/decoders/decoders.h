#ifndef SUM_PRODUCT_DECODER_H
#define SUM_PRODUCT_DECODER_H

#include <Eigen/Core>
#include <Eigen/Sparse>
#include "GF2.hpp"
#include "ldpc-utils.hpp"


enum class LDPC_algo{SP, MS, NMS, LMS, LNMS};

using llr_spmmap_in_it = Eigen::Map<Eigen::SparseMatrix<LLR, Eigen::RowMajor>>::InnerIterator;


std::vector<GF2> hard_decision(std::vector<double> const& llrs);

auto make_ab(Eigen::SparseMatrix<GF2, Eigen::RowMajor> const& H) -> std::tuple<std::vector<std::vector<size_t>>, std::vector<std::vector<size_t>>>;

Eigen::VectorX<GF2> decode_sum_product(Eigen::SparseMatrix<GF2, Eigen::RowMajor> H, std::vector<double> const& R, size_t max_iters = 50, bool verbose = false);

Eigen::VectorX<GF2> decode_sum_product_opt(Eigen::SparseMatrix<GF2, Eigen::RowMajor> H, std::vector<LLR> const& R, size_t max_iters = 50, bool verbose = false);

Eigen::VectorX<GF2> decode_normalized_min_sum(Eigen::SparseMatrix<GF2, Eigen::RowMajor> H, std::vector<LLR> const& R, double scale = 1.0, size_t max_iters = 50, bool verbose = false);

Eigen::VectorX<GF2> decode_normalized_min_sum_opt(Eigen::SparseMatrix<GF2, Eigen::RowMajor> H, std::vector<LLR> const& R, double scale = 1.0, size_t max_iters = 50, bool verbose = false);

Eigen::VectorX<GF2> decode_layered_normalized_min_sum(Eigen::SparseMatrix<GF2, Eigen::RowMajor> H, std::vector<LLR> const& R, size_t layer_size, double scale = 1.0, size_t max_iters = 50, bool verbose = false);

Eigen::VectorX<GF2> decode_sp_to_syndrome(Eigen::SparseMatrix<GF2, Eigen::RowMajor> H, std::vector<LLR> const& R, Eigen::VectorX<GF2> const& s, size_t max_iters = 50, bool verbose = false);

Eigen::VectorX<GF2> decode_nms_to_syndrome(Eigen::SparseMatrix<GF2, Eigen::RowMajor> const& H, std::vector<LLR> const& R, Eigen::VectorX<GF2> const& s, double scale = 1.0, size_t max_iters = 50, bool verbose = false);

Eigen::VectorX<GF2> decode_nms_to_syndrome_r(Eigen::SparseMatrix<GF2, Eigen::RowMajor> const& H, std::vector<LLR> const& R, Eigen::VectorX<GF2> const& s, MemoryManager const& mm, size_t thread_index, double scale = 1.0, size_t max_iters = 50, bool verbose = false);

Eigen::VectorX<GF2> decode_nms_to_syndrome_opt(Eigen::SparseMatrix<GF2, Eigen::RowMajor> H, std::vector<LLR> const& R, Eigen::VectorX<GF2> const& s, double scale = 1.0, size_t max_iters = 50, bool verbose = false);

Eigen::VectorX<GF2> decode_lnms_to_syndrome(Eigen::SparseMatrix<GF2, Eigen::RowMajor> H, std::vector<LLR> const& R, Eigen::VectorX<GF2> const& s, size_t layer_size, double scale = 1.0, size_t max_iters = 50, bool verbose = false);

#endif