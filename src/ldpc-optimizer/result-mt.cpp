#include <iostream>
#include <random>
#include <string>
#include <cmath>
#include "result-mt.h"

// void assign_intersection(Result &res) {
//     double constexpr line{0.001};

//     if (bool res1_under_line{all_under_line(res.result.fers, line)}) {
//         res.intersection_metric = __DBL_MAX__;
// 		std::cout << "metric " << "__DBL_MAX__" << std::endl;
//     } else {
//         res.intersection_metric = find_intersection_point(res.result.bers, res.result.fers, line);
// 		std::cout << "metric " << res.intersection_metric << std::endl;
//     }
// 	// auto rng = std::default_random_engine{};
// 	// size_t seed{std::chrono::steady_clock::now().time_since_epoch().count()};
// 	// rng.seed(seed);
// 	// res.intersection_metric = std::uniform_real_distribution<>(0, 1)(rng);
// }

std::ostream &operator<<(std::ostream &stream, Result &obj) {
        
	int table_rows = std::max({obj.result.bers.size(), obj.result.fers.size(), obj.result.fer_std_devs.size()});
	
	auto comp_function = [](double left, double right) {
		return std::to_string(left).size() < std::to_string(right).size();
	};

	// determine max width of each column ("2+" for 1 prefix and 1 suffix whitespaces)
	int num_col_width = 2+std::to_string(table_rows).size();

	int bers_col_width = 2+std::to_string(*std::max_element(obj.result.bers.begin(),
																obj.result.bers.end(), comp_function)).size();

	int fers_col_width = 2+std::to_string(*std::max_element(obj.result.fers.begin(),
																obj.result.fers.end(), comp_function)).size();

	int std_dev_col_width = 2+std::to_string(*std::max_element(obj.result.fer_std_devs.begin(),
																obj.result.fer_std_devs.end(), comp_function)).size();

	std::string num_cell_str = "N";
	std::string bers_cell_str = "BERS";
	std::string fers_cell_str = "FERS";
	std::string std_dev_cell_str = "FER_STD_DEVS";
	bers_col_width = std::max(2+bers_cell_str.size(), (size_t)bers_col_width);
	fers_col_width = std::max(2+fers_cell_str.size()+2, (size_t)fers_col_width);
	std_dev_col_width = std::max(2+std_dev_cell_str.size()+2, (size_t)std_dev_col_width);

	for (int row = -1; row < table_rows; row++) {
		
		if (row > -1) {
			num_cell_str = std::to_string(row);
			bers_cell_str = row < obj.result.bers.size() ? std::to_string(obj.result.bers.at(row)) : "-";
			fers_cell_str = row < obj.result.fers.size() ? std::to_string(obj.result.fers.at(row)) : "-";
			std_dev_cell_str = row < obj.result.fer_std_devs.size() ? std::to_string(obj.result.fer_std_devs.at(row)) : "-";
		}

		stream << "|" << std::string(num_col_width-1-num_cell_str.size(), ' ') << num_cell_str << " " <<
						"|" << std::string(bers_col_width-1-bers_cell_str.size(), ' ') << bers_cell_str << " " <<
						"|" << std::string(fers_col_width-1-fers_cell_str.size(), ' ') << fers_cell_str << " " <<
						"|" << std::string(std_dev_col_width-1-std_dev_cell_str.size(), ' ') << std_dev_cell_str << " " <<
						"|" << std::endl;
		
	}
	return stream;
}


bool all_under_line(std::vector<double> const& values, double line)
// Проверяет, что все числа вектора находятся под пороговой линией
{
	for (double value : values) {
		if (value > line) {
			return false;
		}
	}

	return true;
}


double sum_distance_from_threshold(std::vector<double> const& values, double line)
// Вычисляет сумму расстояний всех точек вектора от пороговой линии
{
	double distance{0};

	for (double value : values) {
		if (value > line) {
			throw std::runtime_error{"Value is over the threshold"};
		}
		distance += (line - value);
	}

	return distance;
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


bool Result::operator<(Result const& right) const
{
	double constexpr line{0.001};
	double constexpr double_epsilon{0.00001};

	// Checking if results are compatible (represent same experiment)

	if (result.bers.size() != right.result.bers.size()) {
		throw std::runtime_error{"Incompatible results: " + std::to_string(this->result.bers.size()) + " " + std::to_string(right.result.bers.size())};
	}
	for (size_t j{0}; j < result.bers.size(); ++j) {
		if ( std::abs(result.bers[j] - right.result.bers[j]) > double_epsilon ) {
			throw std::runtime_error{"Incompatible BERs in results: " + std::to_string(result.bers[j]) + " " + std::to_string(right.result.bers[j])};
		}
	}

	bool res1_under_line{all_under_line(this->result.fers, line)};
	bool res2_under_line{all_under_line(right.result.fers, line)};

	if (res1_under_line && res2_under_line) {
		return sum_distance_from_threshold(this->result.fers, line) < sum_distance_from_threshold(right.result.fers, line);
	}

	if (res1_under_line) {
		return false;
	}

	if (res2_under_line) {
		return true;
	}

	return this->intersection_metric < right.intersection_metric;
}


bool Result::operator>(Result const& right) const
{
	double constexpr line{0.001};
	double constexpr double_epsilon{0.00001};

	// Checking if results are compatible (represent same experiment)

	if (result.bers.size() != right.result.bers.size()) {
		throw std::runtime_error{"Incompatible results: " + std::to_string(this->result.bers.size()) + " " + std::to_string(right.result.bers.size())};
	}
	for (size_t j{0}; j < result.bers.size(); ++j) {
		if  ( std::abs(result.bers[j] - right.result.bers[j]) > double_epsilon ) {
			throw std::runtime_error{"Incompatible BERs in results: " + std::to_string(result.bers[j]) + " " + std::to_string(right.result.bers[j])};
		}
	}

	bool res1_under_line{all_under_line(this->result.fers, line)};
	bool res2_under_line{all_under_line(right.result.fers, line)};

	if (res1_under_line && res2_under_line) {
		return sum_distance_from_threshold(this->result.fers, line) > sum_distance_from_threshold(right.result.fers, line);
	}

	if (res1_under_line) {
		return true;
	}

	if (res2_under_line) {
		return false;
	}

	return this->intersection_metric > right.intersection_metric;
}


Result compute_average_result(std::vector<Result> const& results)
{
	if (results.empty()) {
		throw std::runtime_error{"Empty results vector"};
	}

	// Checking if results are compatible (represent same experiment)

	for (size_t i{0}; i < results.size() - 1; ++i) {
		if (results[i].result.bers.size() != results[i + 1].result.bers.size()) {
			throw std::runtime_error{"Incompatible results 3"};
		}
		for (size_t j{0}; j < results[i].result.bers.size(); ++j) {
			if (results[i].result.bers[j] != results[i + 1].result.bers[j]) {
				throw std::runtime_error{"Incompatible BERs in results"};
			}
		}		
	}

	Result average_res;

	average_res.result.bers = results[0].result.bers;

	for (size_t j{0}; j < results[0].result.fers.size(); ++j) {
		std::vector<double> fers_for_one_dot;
		fers_for_one_dot.reserve(results.size());

		for (size_t i{0}; i < results.size() - 1; ++i) {
			fers_for_one_dot.push_back(results[i].result.fers[j]);
		}

		auto [mean_fer, fer_std_dev] = compute_mean_and_std(fers_for_one_dot);
		average_res.result.fers.push_back(mean_fer);
		average_res.result.fer_std_devs.push_back(fer_std_dev);
	}

	return average_res;
}