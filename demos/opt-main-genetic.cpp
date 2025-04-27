#include "ldpc-utils.hpp"
#include "genetic_algo_utils.hpp"
#include "genetic_algo.hpp"
#include "file-processor.h"
#include "cxxopts.hpp"

#include <Eigen/Sparse>
#include <random>
#include <algorithm>
#include <utility>
#include <filesystem>
#include <chrono>

#ifndef CMAKE_BINARY_DIR
#define CMAKE_BINARY_DIR ""
#endif

void print_parameters(size_t popul_size, double P_m, std::string input_mat_path, std::string output_mat_path, std::string log_path, size_t Z, std::pair<double, double> QBER_range, double QBER_step, size_t mu, size_t iter_amount, LogSender &logger) {

    std::stringstream ss;
    ss << "Launch parameters: " << std::endl;
    ss << "size_t popul_size = " << popul_size << ";" << std::endl;
    ss << "double P_m = " << P_m << ";" << std::endl;
    ss << "std::string input_mat_path = " << input_mat_path << std::endl;
    ss << "std::string output_mat_path = " << output_mat_path << std::endl;
    ss << "std::string log_path = " << log_path << std::endl;
    ss << "size_t Z = " << Z << ";" << std::endl;
    ss << "std::pair<double, double> QBER_range = {" << QBER_range.first << ", " << QBER_range.second << "};" << std::endl;
    ss << "double QBER_step = " << QBER_step << ";" << std::endl;
    ss << "size_t mu = " << mu << ";" << std::endl;
    ss << "size_t iter_amount = " << iter_amount << ";" << std::endl;
    ss << std::string(7, '\n');
    ss.flush();

    generateLogs_console(logger, "INFO", "genetic_optimizer", "population_generation", ss.str());
}

int main(int argc, char* argv[]) {
    try {

        std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();

        cxxopts::Options options("Genetic optimization", "Genetic optimization");
        options.add_options()
        ("h,help", "Help info", cxxopts::value<bool>()->default_value("false"))
        ("ps", "popul_size", cxxopts::value<size_t>())
        ("pm", "P_m", cxxopts::value<double>())
        ("im", "base graph mat_path", cxxopts::value<std::string>())
        ("om", "optimized mat_path", cxxopts::value<std::string>())
        ("lp", "log_path", cxxopts::value<std::string>())
        ("z", "Z", cxxopts::value<size_t>())
        ("qstart", "QBER_start", cxxopts::value<double>())
        ("qstop", "QBER_stop", cxxopts::value<double>())
        ("qstep", "QBER_step", cxxopts::value<double>())
        ("mu", "mu", cxxopts::value<size_t>())
        ("i", "iter_amount", cxxopts::value<size_t>())
        ;
        auto parsed_opts = options.parse(argc, argv);

        if (parsed_opts.arguments().size() == 0 or parsed_opts["h"].count() > 0) {
            std::cout << options.help() << std::endl;
            return 0;
        }

        size_t popul_size = parsed_opts["ps"].as<size_t>();
        double P_m = parsed_opts["pm"].as<double>();
        std::string input_mat_path = parsed_opts["im"].as<std::string>();
        std::string output_mat_path = parsed_opts["om"].as<std::string>();
        std::string log_path = parsed_opts["lp"].as<std::string>();
        size_t Z = parsed_opts["z"].as<size_t>();
        std::pair<double, double> QBER_range = {parsed_opts["qstart"].as<double>(), parsed_opts["qstop"].as<double>()};
        double QBER_step = parsed_opts["qstep"].as<double>();
        size_t mu = parsed_opts["mu"].as<size_t>();
        size_t iter_amount = parsed_opts["i"].as<size_t>();

        if (input_mat_path == output_mat_path)
            throw std::runtime_error("Input and output matrixes paths are the same (" + input_mat_path + ")");

            
        LogSender logger;
        logger.addConsoleReceiver(LogLevel::ALL);

        
        print_parameters(popul_size, P_m, input_mat_path, output_mat_path, log_path, Z, QBER_range, QBER_step, mu, iter_amount, logger);
        
        CSVReceiverT<std::string, std::string, std::string, std::string, std::string> csv(
            log_path, {"timestamp", "level", "source", "type", "message"}
        );

        std::multimap<size_t, std::multimap<IntersectionMetric, GeneticMatrix, std::greater<IntersectionMetric>>> some_res = genetic_algo(popul_size, P_m, input_mat_path, Z, QBER_range, mu, iter_amount, logger, csv);


        // print metric of default matrix
        auto mat_stem = std::filesystem::path(input_mat_path).stem().string();
        IntersectionMetric origin_mat_metric;
        MyMatrix origin_mat_matrix;
        for (auto res : some_res.begin()->second) {
            if (res.second.history == mat_stem) {
                origin_mat_metric = res.first;
                origin_mat_matrix = res.second.matrix;
            }
        }
        
        generateLogs_console(logger, "DATA", "genetic_optimizer", "plain_results", mat_stem + " metric: " + std::to_string(origin_mat_metric));
        benchmarks::BSChannellWynersEC origin_busc_bm{origin_mat_matrix};
        Result origin_obj_func{origin_busc_bm.run(QBER_range.first, QBER_range.second, QBER_step, LDPC_algo::NMS, false)};
        
        std::stringstream ss;
        ss << origin_obj_func;
        generateLogs_console(logger, "DATA", "genetic_optimizer", "plain_results", ss.str());
        ss.str("");


        IntersectionMetric best_metric = some_res.begin()->second.begin()->first;
        MyMatrix best_matrix = some_res.begin()->second.begin()->second.matrix;
        for (auto popul = some_res.begin(); popul != some_res.end(); popul++) {
            
            if (best_metric < popul->second.begin()->first) {
                best_metric = popul->second.begin()->first;
                best_matrix = popul->second.begin()->second.matrix;
            }

        }
        
        generateLogs_console(logger, "DATA", "genetic_optimizer", "plain_results", "All epochs best metric: " + std::to_string(best_metric));
        benchmarks::BSChannellWynersEC best_busc_bm{best_matrix};
        Result best_obj_func{best_busc_bm.run(QBER_range.first, QBER_range.second, QBER_step, LDPC_algo::NMS, false)};
        
        ss << best_obj_func;
        generateLogs_console(logger, "DATA", "genetic_optimizer", "plain_results", ss.str());

        dump_matrix(best_matrix, output_mat_path);


        std::chrono::steady_clock::time_point end_time = std::chrono::steady_clock::now();

        auto duration = end_time-start_time;
        auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
        duration -= hours;
        auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration);
        duration -= minutes;
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
        
        generateLogs_console(logger, "INFO", "genetic_optimizer", "exec_time",
        "Running time: " + std::to_string(hours.count()) + "h " + std::to_string(minutes.count()) + "m " + std::to_string(seconds.count()) + "s");
    
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
