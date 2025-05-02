#include "ldpc-utils.hpp"
#include "file-processor.h"
#include "result-mt.h"
#include "LogSender.h"
#include "cxxopts.hpp"

#include <iostream>
#include <random>
#include <taskflow/taskflow.hpp>
#include <chrono>
#include <numeric>
#include <string>

using IntersectionMetric = double;


void generateLogs_console(LogSender &sender, std::string severity, std::string source, std::string messageType, std::string message) {
    
    std::string timestamp = getCurrentTimeStr();
    std::string logMessage = "[" + timestamp + "] " + severity + " " + source + "," + messageType + ": " + message;

    sender << std::pair{LogLevel::ALL, logMessage};
}

void generateLogs_csv(CSVReceiverT<std::string, std::string, std::string, std::string, std::string> &csv,
						std::string severity, std::string source, std::string messageType, std::string message) {
    
    std::string timestamp = getCurrentTimeStr();
    std::string logMessage = "[" + timestamp + "] " + severity + " " + source + "," + messageType + ": " + message;
    
    csv.writeRow(timestamp, severity, source, messageType, message);
}

void validate_params(std::string input_mat_path, std::string output_mat_path, std::string log_path, size_t Z,
			std::pair<double, double> QBER_range, double QBER_step, size_t epochs, size_t iters, size_t shifts, LogSender &logger) {
	
	std::string err_msg;
	std::string log_msg;
	if (input_mat_path == output_mat_path) {
		err_msg = "Input and output matrixes paths are the same (" + input_mat_path + " == " + output_mat_path + ")";
		generateLogs_console(logger, "ERROR", "greedy_optimizer", "validation_params", "runtime_error: "+err_msg);
		throw std::runtime_error(err_msg);
	} else {
		log_msg = "input_mat_path != output_mat_path: " + input_mat_path + " != " + output_mat_path;
		generateLogs_console(logger, "INFO", "greedy_optimizer", "validation_params", log_msg);
	}

	if (log_path == input_mat_path) {
		err_msg = "Log can`t be saved in same place as base graph matrix: (" + log_path + ")";
		generateLogs_console(logger, "ERROR", "greedy_optimizer", "validation_params", "runtime_error: "+err_msg);
		throw std::runtime_error(err_msg);
	} else {
		log_msg = "Log successfully saved: (" + log_path + ")";
		generateLogs_console(logger, "INFO", "greedy_optimizer", "validation_params", log_msg);
	}

	if (log_path == output_mat_path) {
		err_msg = "Log can`t be saved in same place as optimized matrix: (" + log_path + ")";
		generateLogs_console(logger, "ERROR", "greedy_optimizer", "validation_params", "runtime_error: "+err_msg);
		throw std::runtime_error(err_msg);
	} else {
		log_msg = "Log successfully saved: (" + log_path + ")";
		generateLogs_console(logger, "INFO", "greedy_optimizer", "validation_params", log_msg);
	}

	if (Z < 1) {
		err_msg = "Base graph can`t be enhanced: Z < 1 (" + std::to_string(Z) + " < 1)";
		generateLogs_console(logger, "ERROR", "greedy_optimizer", "validation_params", "runtime_error: "+err_msg);
		throw std::runtime_error(err_msg);
	} else {
		log_msg = "Base graph can be enhanced: Z >= 1 (" + std::to_string(Z) + " >= 1)";
		generateLogs_console(logger, "INFO", "greedy_optimizer", "validation_params", log_msg);
	}

	if (QBER_range.first > QBER_range.second) {
		err_msg = "QBER_start > QBER_stop (" + std::to_string(QBER_range.first) + " > " + std::to_string(QBER_range.second) + ")";
		generateLogs_console(logger, "ERROR", "greedy_optimizer", "validation_params", "runtime_error: "+err_msg);
		throw std::runtime_error(err_msg);
	} else {
		log_msg = "QBER_start <= QBER_stop (" + std::to_string(QBER_range.first) + " <= " + std::to_string(QBER_range.second) + ")";
		generateLogs_console(logger, "INFO", "greedy_optimizer", "validation_params", log_msg);
	}

	if (QBER_step <= 0.0) {
		err_msg = "QBER_step <= 0.0 (" + std::to_string(QBER_step) + " <= 0.0)";
		generateLogs_console(logger, "ERROR", "greedy_optimizer", "validation_params", "runtime_error: "+err_msg);
		throw std::runtime_error(err_msg);
	} else {
		log_msg = "QBER_step > 0.0 (" + std::to_string(QBER_step) + " > 0.0)";
		generateLogs_console(logger, "INFO", "greedy_optimizer", "validation_params", log_msg);
	}

	if (epochs < 1) {
		err_msg = "epochs < 1 (" + std::to_string(epochs) + " < 1)";
		generateLogs_console(logger, "ERROR", "greedy_optimizer", "validation_params", "runtime_error: "+err_msg);
		throw std::runtime_error(err_msg);
	} else {
		log_msg = "epochs >= 1 (" + std::to_string(epochs) + " >= 1)";
		generateLogs_console(logger, "INFO", "greedy_optimizer", "validation_params", log_msg);
	}

	if (iters < 1) {
		err_msg = "iters < 1 (" + std::to_string(iters) + " < 1)";
		generateLogs_console(logger, "ERROR", "greedy_optimizer", "validation_params", "runtime_error: "+err_msg);
		throw std::runtime_error(err_msg);
	} else {
		log_msg = "iters >= 1 (" + std::to_string(iters) + " >= 1)";
		generateLogs_console(logger, "INFO", "greedy_optimizer", "validation_params", log_msg);
	}

	if (shifts < 0) {
		err_msg = "shifts < 0 (" + std::to_string(shifts) + " < 0)";
		generateLogs_console(logger, "ERROR", "greedy_optimizer", "validation_params", "runtime_error: "+err_msg);
		throw std::runtime_error(err_msg);
	} else {
		log_msg = "shifts >= 0 (" + std::to_string(shifts) + " >= 0)";
		generateLogs_console(logger, "INFO", "greedy_optimizer", "validation_params", log_msg);
	}
}

void print_matrix_as_triplets(Eigen::SparseMatrix<GF2, Eigen::RowMajor> const& matrix, LogSender &logger) {
	
	std::stringstream ss;
	generateLogs_console(logger, "DATA", "greedy_optimizer", "results_printing", "ROW\tCOL\tVAL");
	for (int k=0; k < matrix.outerSize(); k++) {
		for (Eigen::SparseMatrix<GF2, Eigen::RowMajor>::InnerIterator it(matrix, k); it; ++it) {			
			ss << it.row() << "\t" << it.col() << "\t" << it.value() << std::endl;
		}
	}
	generateLogs_console(logger, "DATA", "greedy_optimizer", "results_printing", ss.str());
}

void print_parameters(std::string input_mat_path, std::string output_mat_path, std::string log_path, size_t Z,
						std::pair<double, double> QBER_range, double QBER_step, size_t epochs, size_t iters, size_t shifts, LogSender &logger) {

	std::stringstream ss;
	ss << "Launch parameters: " << std::endl;
	ss << "std::string input_mat_path = " << input_mat_path << std::endl;
	ss << "std::string output_mat_path = " << output_mat_path << std::endl;
	ss << "std::string log_path = " << log_path << std::endl;
	ss << "size_t Z = " << Z << ";" << std::endl;
	ss << "std::pair<double, double> QBER_range = {" << QBER_range.first << ", " << QBER_range.second << "};" << std::endl;
	ss << "double QBER_step = " << QBER_step << ";" << std::endl;
	ss << "size_t epochs = " << epochs << ";" << std::endl;
	ss << "size_t iters = " << iters << ";" << std::endl;
	ss << "size_t shifts = " << shifts << ";" << std::endl;
	ss << std::string(7, '\n');
	ss.flush();

	generateLogs_console(logger, "INFO", "greedy_optimizer", "launch_params", ss.str());
}

int main(int argc, char *argv[]) {

	std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();

	cxxopts::Options options("Greedy optimization", "Greedy optimization");
	options.add_options()
	("h,help", "Help info", cxxopts::value<bool>()->default_value("false"))
	("im", "base graph mat_path", cxxopts::value<std::string>())
	("om", "optimized mat_path", cxxopts::value<std::string>())
	("lp", "log_path", cxxopts::value<std::string>())
	("z", "Z", cxxopts::value<size_t>())
	("qstart", "QBER_start", cxxopts::value<double>())
	("qstop", "QBER_stop", cxxopts::value<double>())
	("qstep", "QBER_step", cxxopts::value<double>())
	("epochs", "epoch_amount", cxxopts::value<size_t>())
	("iters", "iter_amount", cxxopts::value<size_t>())
	("shifts", "shift_amount", cxxopts::value<size_t>())
	;
	auto parsed_opts = options.parse(argc, argv);

	if (parsed_opts.arguments().size() == 0 or parsed_opts["h"].count() > 0) {
		std::cout << options.help() << std::endl;
		return 0;
	}

	const std::string input_mat_path = parsed_opts["im"].as<std::string>();
	const std::string output_mat_path = parsed_opts["om"].as<std::string>();
	const std::string log_path = parsed_opts["lp"].as<std::string>();
	const size_t Z = parsed_opts["z"].as<size_t>();
	const std::pair<double, double> QBER_range = {parsed_opts["qstart"].as<double>(), parsed_opts["qstop"].as<double>()};
	const double QBER_step = parsed_opts["qstep"].as<double>();
	const size_t epochs = parsed_opts["epochs"].as<size_t>();
	const size_t iters = parsed_opts["iters"].as<size_t>();
	const size_t shifts = parsed_opts["shifts"].as<size_t>();

	LogSender logger;
    logger.addConsoleReceiver(LogLevel::ALL);
	CSVReceiverT<std::string, std::string, std::string, std::string, std::string> csv(
		log_path, {"timestamp", "level", "source", "type", "message"}
	);


	validate_params(input_mat_path, output_mat_path, log_path, Z, QBER_range, QBER_step, epochs, iters, shifts, logger);

	print_parameters(input_mat_path, output_mat_path, log_path, Z, QBER_range, QBER_step, epochs, iters, shifts, logger);


	BG_type bg_type;
    auto mat_stem = std::filesystem::path(input_mat_path).stem();
    if (mat_stem == "BG1") {
        bg_type = BG_type::BG1;
        generateLogs_console(logger, "INFO", "greedy_optimizer", "matrix_loading", "Matrix`s BG_type == BG1");
    } else if (mat_stem == "BG2") {
        bg_type = BG_type::BG2;
        generateLogs_console(logger, "INFO", "greedy_optimizer", "matrix_loading", "Matrix`s BG_type == BG2");
    } else {
        bg_type = BG_type::NOT_5G;
        generateLogs_console(logger, "INFO", "greedy_optimizer", "matrix_loading", "Matrix`s BG_type == NOT_5G");
    }

	Eigen::SparseMatrix<GF2, Eigen::RowMajor> bg{load_matrix_from_alist(input_mat_path)};
	generateLogs_console(logger, "INFO", "greedy_optimizer", "matrix_loading", mat_stem.generic_string() + " was loaded succesfully");

	// size_t m = bg.rows();
	// size_t n = bg.cols();

	// bg = bg.block(0, 0, 22, 44); // R = 1/2

	std::random_device r;
	std::mt19937 rand_gen(r());

	std::uniform_int_distribution inverse_row_distribution{0, 21}; // ?
	std::uniform_int_distribution inverse_col_distribution{0, 26}; // ?

	Eigen::SparseMatrix<GF2, Eigen::RowMajor> H = enhance_from_base(bg, Z);
	H.makeCompressed();

	// Вычисление начального значения
	Eigen::SparseMatrix<GF2, Eigen::RowMajor> shifted_H = shift_eyes(H, Z, bg_type, shift_randomness::NO_RANDOM);
	shifted_H.makeCompressed();
	benchmarks::BUSChannellWynersEC busc_bm{shifted_H, {0.005, 0.01, 0.02, 0.04}, {-1, -1}, true};
	
	
	std::stringstream ss;
	Result origin_obj_func{busc_bm.run(QBER_range.first, QBER_range.second, QBER_step, LDPC_algo::NMS, false)};
	ss << origin_obj_func;
	generateLogs_console(logger, "DATA", "greedy_optimizer", "plain_results", ss.str());
	ss.str("");

	IntersectionMetric current_res{ busc_bm.find_intersection(QBER_range.first, QBER_range.second, 10e-3, 0.001, LDPC_algo::NMS, false) };
	generateLogs_csv(csv, "DATA", "greedy_optimizer", "results_printing", mat_stem.generic_string() + " metric " + std::to_string(current_res));

	try {
		for (size_t epoch_number{1}; epoch_number <= epochs; ++epoch_number) {
			// std::cout << "Epoch" << epoch_number << std::endl;
			// std::cout.flush();
			generateLogs_csv(csv, "DATA", "greedy_optimizer", "results_printing", "Epoch " + std::to_string(epoch_number));

			for (size_t iter_number{0}; iter_number < iters; ++iter_number) {

				// std::cout << "Iteration " << iter_number << std::endl;
				// std::cout.flush();
				// generateLogs_csv(csv, "DATA", "greedy_optimizer", "results_printing", "Iteration " + std::to_string(iter_number));

				Eigen::SparseMatrix<GF2, Eigen::RowMajor> bg_local{bg};

				for (size_t inv_number{0}; inv_number < epoch_number; ++inv_number) {
					size_t row_to_inverse{inverse_row_distribution(rand_gen)};
					size_t col_to_inverse{inverse_col_distribution(rand_gen)};

					// Инвертируем EPOCHS бит в случайной позиции
					bg_local.coeffRef(row_to_inverse, col_to_inverse) = !bg_local.coeffRef(row_to_inverse, col_to_inverse);
				}
				bg_local.makeCompressed();

				Eigen::SparseMatrix<GF2, Eigen::RowMajor> H_local = enhance_from_base(bg_local, Z);
				H_local.makeCompressed();

				std::vector<IntersectionMetric> results;
				results.reserve(shifts);

				tf::Executor executor;
				tf::Taskflow taskflow;
				std::mutex results_sync;

				for (size_t shift_number{0}; shift_number < shifts; ++shift_number) {
					taskflow.emplace([&, shift_number](){

						Eigen::SparseMatrix<GF2, Eigen::RowMajor> shifted_H_local = shift_eyes(H_local, Z, bg_type, shift_randomness::COMBINE);
						shifted_H_local.makeCompressed();
						benchmarks::BUSChannellWynersEC busc_bm{shifted_H_local, {0.005, 0.01, 0.02, 0.04}, {-1, -1}, true};

						IntersectionMetric res{ busc_bm.find_intersection(QBER_range.first, QBER_range.second, 10e-3, 0.001, LDPC_algo::NMS, false) };
						
						results_sync.lock();
						generateLogs_console(logger, "INFO", "greedy_optimizer", "calculation_obj_func", "obj_func was calculated for shift_number " + std::to_string(shift_number));
						results.push_back(res);
						results_sync.unlock();
					});
				}

				executor.run(taskflow).wait();

				IntersectionMetric av_result{ (IntersectionMetric)((double)(std::accumulate(results.begin(), results.end(), 0.0)) / (double)(results.size())) };

				// std::cout << av_result << std::endl;
				generateLogs_csv(csv, "DATA", "greedy_optimizer", "results_printing", "Iteration " + std::to_string(iter_number) + " metric " + std::to_string(av_result));

				if (current_res < av_result) {
					// std::cout << "Better matrix found\n";
					generateLogs_console(logger, "DATA", "greedy_optimizer", "results_printing", "Better matrix found");
					print_matrix_as_triplets(bg_local, logger);
					// std::cout.flush();
					bg = bg_local;
					current_res = av_result;
				}
			} // for (size_t iter_number{0}; iter_number < ITERATIONS; ++iter_number)
		} // for (size_t epoch_number{0}; epoch_number < EPOCHS; ++epoch_number)
		
		benchmarks::BUSChannellWynersEC best_busc_bm{bg, {0.005, 0.01, 0.02, 0.04}, {-1, -1}, true};
        Result best_obj_func{best_busc_bm.run(QBER_range.first, QBER_range.second, QBER_step, LDPC_algo::NMS, false)};
        ss << best_obj_func;
        generateLogs_console(logger, "DATA", "greedy_optimizer", "plain_results", ss.str());
		ss.str("");

		
		dump_matrix(bg, output_mat_path);
		generateLogs_console(logger, "INFO", "greedy_optimizer", "matrix_dumping", "Matrix was dumped");


        std::chrono::steady_clock::time_point end_time = std::chrono::steady_clock::now();

        auto duration = end_time-start_time;
        auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
        duration -= hours;
        auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration);
        duration -= minutes;
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);

		generateLogs_console(logger, "INFO", "greedy_optimizer", "exec_time",
			"Running time: " + std::to_string(hours.count()) + "h " + std::to_string(minutes.count()) + "m " + std::to_string(seconds.count()) + "s");

	} // try


	catch (std::runtime_error err) {
		std::cerr << "Exception occured: " << err.what() << std::endl;
	}
	

	return 0;
}