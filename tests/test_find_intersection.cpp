#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "benchmarks.h"
#include "file-processor.h"
#include <doctest/doctest.h>
#include <fstream>
#include <random>
#include <stdexcept>

using namespace benchmarks;
std::pair<double, double>
manual_intersection(const BaseBenchmark::RunningResult &res1,
										const BaseBenchmark::RunningResult &res2) {
	for (size_t i = 0; i < res1.bers.size() - 1; ++i) {
		double x1 = res1.bers[i], y1 = res1.fers[i];
		double x2 = res1.bers[i + 1], y2 = res1.fers[i + 1];
		double x3 = res2.bers[i], y3 = res2.fers[i];
		double x4 = res2.bers[i + 1], y4 = res2.fers[i + 1];

		double denom = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
		if (std::abs(denom) < 1e-10)
			continue;

		double x =
				((x1 * y2 - y1 * x2) * (x3 - x4) - (x1 - x2) * (x3 * y4 - y3 * x4)) /
				denom;
		double y =
				((x1 * y2 - y1 * x2) * (y3 - y4) - (y1 - y2) * (x3 * y4 - y3 * x4)) /
				denom;

		if (x >= std::min(x1, x2) && x <= std::max(x1, x2) &&
				x >= std::min(x3, x4) && x <= std::max(x3, x4)) {
			return {x, y};
		}
	}
	return {-1, -1};
}


double find_intersection_point(std::vector<double> const& x_values, std::vector<double> const& y_values, double line)
{
	for (size_t i{0}; i < y_values.size(); ++i) {
		if (y_values[i] > line) {
			double x1{x_values[i - 1]}, y1{y_values[i - 1]}, x2{x_values[i]}, y2{y_values[i]};
			double x3{x_values[i - 1]}, y3{line}, x4{x_values[i]}, y4{line};
			return ((x1 * y2 - y1 * x2) * (x3 - x4) - (x1 - x2) * (x3 * y4 - y3 * x4)) / ((x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4));
		}
	}
}


Eigen::SparseMatrix<GF2, Eigen::RowMajor>
load_matrix_with_check(const std::string &filename) {
	std::ifstream file(filename);
	if (!file.good()) {
		throw std::runtime_error("File not found: " + filename);
	}
	return load_matrix_from_alist(filename);
}

class MockWynersEC : public BSChannellWynersEC 
{
public:
MockWynersEC(std::string const &H_name, BG_type bg_type, size_t bg_rows, size_t bg_cols, size_t Z, double threshold)
	:BSChannellWynersEC(H_name, bg_type, bg_rows, bg_cols, Z), gen{42},	dist{0.0, 1.0}, m_threshold{threshold} {}

	auto perform_error_correction(double ber, LDPC_algo alg_type, MemoryManager const& mm, size_t thread_index) -> bool const override 
	{
		// Вычисление FER на основе BER
		double fer = (m_threshold / 10) * std::exp(50 * ber);

		double random_number{dist(gen)};

		return random_number > fer;
	}

private:
	mutable std::mt19937 gen;
	mutable std::uniform_real_distribution<> dist;
	double m_threshold;
};

TEST_CASE("BG1: MockWynersEC find_intersection comparison with run method") {
	try {

		double ber_start = 0.0;
		double ber_stop = 0.1;
		double ber_step = 0.0001;
		double ber_prec = 0.0001;
		double threshold = 0.001;
		LDPC_algo alg_type = LDPC_algo::SP;
		bool verbose = false;

		MockWynersEC wynersEC("BG1.alist", BG_type::BG1, 46, 68, 4, threshold);

		double intersection_ber = wynersEC.find_intersection(ber_start, ber_stop, ber_prec, threshold, alg_type, verbose);

		auto result1 = wynersEC.run(ber_start, ber_stop, ber_step, alg_type, verbose);

		double manual_intersection_ber{find_intersection_point(result1.bers, result1.fers, threshold)};

		MESSAGE("find_intersection result: ", intersection_ber);
		MESSAGE("manual intersection: BER=", manual_intersection_ber);

		REQUIRE(manual_intersection_ber > 0);

		CHECK(intersection_ber == doctest::Approx(manual_intersection_ber).epsilon(ber_prec / 10));
	} catch (const std::exception &e) {
		FAIL(e.what());
	}
}

