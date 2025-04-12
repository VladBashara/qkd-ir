#ifndef BENCHMARKS_H
#define BENCHMARKS_H

#include "decoders.h"
#include "ldpc-utils.hpp"

#include <thread>
#include <mutex>
#include <array>
#include <map>


namespace benchmarks 
{

class BaseBenchmark
{
public:
	struct RunningResult
	{
		std::vector<double> bers;
		std::vector<double> fers;
		std::vector<double> fer_std_devs;
	};

	BaseBenchmark(Eigen::SparseMatrix<GF2, Eigen::RowMajor> const& H);
	BaseBenchmark(std::string const& H_name, BG_type bg_type, size_t bg_rows, size_t bg_cols, size_t Z);
	auto run(double ber_start, double ber_stop, double ber_step, LDPC_algo alg_type, bool verbose) -> RunningResult const;
	auto virtual perform_error_correction(double ber, LDPC_algo alg_type, MemoryManager const& mm, size_t thread_index) -> bool const = 0;
	auto find_intersection(double ber_start, double ber_stop, double ber_prec, double threshold, LDPC_algo alg_type, bool verbose) -> double const;
	void change_m_H(std::vector<std::pair<int, int>> changes);

protected:
	auto virtual compute_llrs(Eigen::Vector<double, Eigen::Dynamic> const& received_data, double ber) -> std::vector<LLR> const = 0;
	auto virtual add_errors(Eigen::Vector<GF2, Eigen::Dynamic> const& codeword, double ber) -> Eigen::Vector<double, Eigen::Dynamic> const = 0;
	auto gen_rand_bit_seq(size_t len) -> Eigen::Vector<GF2, Eigen::Dynamic> const;

	Eigen::SparseMatrix<GF2, Eigen::RowMajor> m_H;
	size_t m_Z;

private:
	auto compute_one_point(double ber, LDPC_algo alg_type, MemoryManager const& mm, size_t const STAT_ITERATIONS, bool verbose) -> std::pair<double, double>;
};


class ClassicEC : public BaseBenchmark
{
public:
	ClassicEC(std::string const& H_name, BG_type bg_type, size_t bg_rows, size_t bg_cols, size_t Z) : BaseBenchmark{H_name, bg_type, bg_rows, bg_cols, Z} {}
	ClassicEC(Eigen::SparseMatrix<GF2, Eigen::RowMajor> const& H) : BaseBenchmark{H} {}
	auto perform_error_correction(double ber, LDPC_algo alg_type, MemoryManager const& mm, size_t thread_index) -> bool const override;
private:
	auto encode(Eigen::Vector<GF2, Eigen::Dynamic> const& message) -> Eigen::Vector<GF2, Eigen::Dynamic> const;
};


class BSChannellEC : public ClassicEC
{
public:
	BSChannellEC(std::string const& H_name, BG_type bg_type, size_t bg_rows, size_t bg_cols, size_t Z) : ClassicEC{H_name, bg_type, bg_rows, bg_cols, Z} {}
protected: 
	auto compute_llrs(Eigen::Vector<double, Eigen::Dynamic> const& received_data, double ber) -> std::vector<LLR> const override;
	auto add_errors(Eigen::Vector<GF2, Eigen::Dynamic> const& codeword, double ber) -> Eigen::Vector<double, Eigen::Dynamic> const override;
};


class BIAWGNChannellEC : public ClassicEC
{
public:
	BIAWGNChannellEC(std::string const& H_name, BG_type bg_type, size_t bg_rows, size_t bg_cols, size_t Z) : ClassicEC{H_name, bg_type, bg_rows, bg_cols, Z} {}
protected: 
	auto compute_llrs(Eigen::Vector<double, Eigen::Dynamic> const& received_data, double ber) -> std::vector<LLR> const override;
	auto add_errors(Eigen::Vector<GF2, Eigen::Dynamic> const& codeword, double ber) -> Eigen::Vector<double, Eigen::Dynamic> const override;
private:
	auto ber_to_sigma(double ber) -> double const;
};


class WynersEC : public BaseBenchmark
{
public:
	WynersEC(std::string const& H_name, BG_type bg_type, size_t bg_rows, size_t bg_cols, size_t Z) : BaseBenchmark{H_name, bg_type, bg_rows, bg_cols, Z} {}
	WynersEC(Eigen::SparseMatrix<GF2, Eigen::RowMajor> const& H) : BaseBenchmark{H} {}
	auto perform_error_correction(double ber, LDPC_algo alg_type, MemoryManager const& mm, size_t thread_index) -> bool const override;
};


class BSChannellWynersEC : public WynersEC
{
public:
	BSChannellWynersEC(std::string const& H_name, BG_type bg_type, size_t bg_rows, size_t bg_cols, size_t Z) : WynersEC{H_name, bg_type, bg_rows, bg_cols, Z} {}
	BSChannellWynersEC(Eigen::SparseMatrix<GF2, Eigen::RowMajor> const& H) : WynersEC{H} {}
protected: 
	auto compute_llrs(Eigen::Vector<double, Eigen::Dynamic> const& received_data, double ber) -> std::vector<LLR> const override;
	auto add_errors(Eigen::Vector<GF2, Eigen::Dynamic> const& codeword, double ber) -> Eigen::Vector<double, Eigen::Dynamic> const override;
};


class BUSChannellWynersEC : public WynersEC
{
public:
	typedef std::array<std::array<double, 2>, 2> error_distribution_t; // dimensions 1 -- bit, 2 -- detector; basis is always 0 for now

	BUSChannellWynersEC(std::string const& H_name, BG_type bg_type, size_t bg_rows, size_t bg_cols, size_t Z, error_distribution_t error_distrib, std::pair<GF2, GF2> changing_err_index, bool modify_llrs) : WynersEC{H_name, bg_type, bg_rows, bg_cols, Z}, 
		m_error_distribution{error_distrib}, m_initial_error_distribution{error_distrib}, m_changing_err_index{changing_err_index}, m_modify_llrs{modify_llrs}
		{
			if (m_error_distribution.empty()) {
				throw std::runtime_error{"BUSChannellWynersEC::BUSChannellWynersEC: don't pass empty error distribution!"};
			}
		}
	BUSChannellWynersEC(Eigen::SparseMatrix<GF2, Eigen::RowMajor> const& H, error_distribution_t error_distrib, std::pair<int, int> changing_err_index, bool modify_llrs) : WynersEC{H},
		m_error_distribution{error_distrib}, m_initial_error_distribution{error_distrib}, m_changing_err_index{changing_err_index}, m_modify_llrs{modify_llrs}
		{
			if (m_error_distribution.empty()) {
				throw std::runtime_error{"BUSChannellWynersEC::BUSChannellWynersEC: don't pass empty error distribution!"};
			}
		}
protected: 
	auto compute_llrs(Eigen::Vector<double, Eigen::Dynamic> const& received_data, double ber) -> std::vector<LLR> const override;
	auto add_errors(Eigen::Vector<GF2, Eigen::Dynamic> const& codeword, double ber) -> Eigen::Vector<double, Eigen::Dynamic> const override;
private:
	error_distribution_t m_error_distribution;	
	error_distribution_t const m_initial_error_distribution;	
	std::map<std::thread::id, std::vector<GF2>> m_bit_group_distribution; // GF2 stands for detector number
	std::mutex m_bit_group_distribution_mutex;
	bool m_modify_llrs;
	std::pair<int, int> m_changing_err_index; // pair stands for bit and detector, {-1, -1} means change all errors
};


const double laplace_z_value_confidence90 = 1.65;
const double laplace_z_value_confidence95 = 1.96;
const double laplace_z_value_confidence99 = 2.58;

class BenchmarkRange
{
public:
    double start;
    double stop;
    double step;
    int length;
    BenchmarkRange(double _start, double _stop, double _step);
};


class ExposedBUSChannellWynersEC : public BSChannellWynersEC
{
public:
    ExposedBUSChannellWynersEC(std::string const& H_name, BG_type bg_type=BG_type::BG1, size_t bg_rows=22, size_t bg_cols=44, size_t Z=4) : BSChannellWynersEC(H_name, bg_type, bg_rows, bg_cols, Z){

    }

    struct ExposedRunningResult
    {
        std::vector<double> bers;
        std::vector<double> exposed_rates;
        std::vector<double> fers;
        std::vector<double> fer_std_devs;
        std::vector<double> estimated_bers;
        std::vector<double> estimated_ber_std_devs;

    };
    auto perform_error_correction(double ber, double exposed_bits_rate, LDPC_algo alg_type, double &estimated_ber, MemoryManager const& mm, size_t thread_index) -> bool const;
    auto run(BenchmarkRange ber_range, BenchmarkRange exposed_rate_range, LDPC_algo alg_type, bool verbose) -> ExposedRunningResult const;
};


}

std::pair<double, double> compute_mean_and_std(std::vector<double> const& values);
std::pair<double, double> compute_bernoulli_p_confidence_interval(std::vector<double> const &values);
double estimate_ber_by_exposed(Eigen::Vector<GF2, Eigen::Dynamic> const &codeword, Eigen::Vector<double, Eigen::Dynamic> const &received_data, double exposed_bits_rate);


#endif