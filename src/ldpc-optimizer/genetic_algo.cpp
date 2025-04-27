#include "ldpc-utils.hpp"
#include "file-processor.h"
#include "genetic_algo_utils.hpp"
#include "genetic_algo.hpp"
#include "result-mt.h"
#include "LogSender.h"

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <random>
#include <iterator>
#include <functional>
#include <cmath>
#include <sstream>
#include <filesystem>
#include <chrono>
#include <taskflow/taskflow.hpp>

void generateLogs_console(LogSender &sender, std::string severity, std::string source, std::string messageType, std::string message) {
    
    std::string timestamp = getCurrentTimeStr();
    std::string logMessage = "[" + timestamp + "] " + severity + " " + source + "," + messageType + ": " + message;

    sender << std::pair{LogLevel::ALL, logMessage};
}

void generateLogs_csv(CSVReceiverT<std::string, std::string, std::string, std::string, std::string> &csv, std::string severity, std::string source, std::string messageType, std::string message) {
    
    std::string timestamp = getCurrentTimeStr();
    std::string logMessage = "[" + timestamp + "] " + severity + " " + source + "," + messageType + ": " + message;
    
    csv.writeRow(timestamp, severity, source, messageType, message);
}

size_t get_seed() {
    return std::chrono::steady_clock::now().time_since_epoch().count();
}

std::multimap<IntersectionMetric, GeneticMatrix, std::greater<IntersectionMetric>> calculate_obj_func(
    const BG_type bg_type, const std::multimap<IntersectionMetric, GeneticMatrix, std::greater<IntersectionMetric>> &population_map, const std::pair<double, double> QBER_range,
    const size_t Z, LogSender &logger) {

    std::multimap<IntersectionMetric, GeneticMatrix, std::greater<IntersectionMetric>> results_map;

    constexpr IntersectionMetric DEFAULT_KEY = -1.0;

    tf::Executor executor;
    tf::Taskflow taskflow;
    std::mutex results_sync;
    
    for (auto &[metric, gen_matrix] : population_map) { 

        if ( metric != DEFAULT_KEY ) { // skip already calculated values
            results_map.insert(std::pair(metric, gen_matrix));
            continue;
        }

        taskflow.emplace([&](){

            GeneticMatrix expanded_mat = {enhance_from_base(gen_matrix.matrix, Z), gen_matrix.history, gen_matrix.matrix_id};
            expanded_mat = {shift_eyes(expanded_mat.matrix, Z, bg_type, shift_randomness::COMBINE), gen_matrix.history, gen_matrix.matrix_id};
            expanded_mat.matrix.makeCompressed();

            // benchmarks::BSChannellWynersEC busc_bm{expanded_mat.matrix};
            benchmarks::BUSChannellWynersEC busc_bm{expanded_mat.matrix, {0.005, 0.01, 0.02, 0.04}, {-1, -1}, true};

            IntersectionMetric obj_func = busc_bm.find_intersection(QBER_range.first, QBER_range.second, 10e-3, 0.001, LDPC_algo::NMS, false);

            results_sync.lock();
            generateLogs_console(logger, "DATA", "genetic_optimizer", "calculation_obj_func", "obj_func was calculated for " + gen_matrix.matrix_id);
            results_map.insert(std::pair(obj_func, gen_matrix));
            results_sync.unlock();
        });
    }

    executor.run(taskflow).wait();

    return results_map;
}

std::multimap<size_t, std::multimap<IntersectionMetric, GeneticMatrix, std::greater<IntersectionMetric>>> genetic_algo(const size_t popul_size, const double P_m, const std::string mat_path, const size_t Z,
                  const std::pair<double, double> QBER_range, const size_t mu, const size_t iter_amount, LogSender &logger, CSVReceiverT<std::string, std::string, std::string, std::string, std::string> &csv) {
    
    generateLogs_console(logger, "INFO", "genetic_optimizer", "validation_state", "Start validation"); 
    
    std::string err_msg;
    if (popul_size <= 2) {
        err_msg = "popul_size <= 2 : " + std::to_string(popul_size) + " <= 2";
        generateLogs_console(logger, "ERROR", "genetic_optimizer", "validation_state", "runtime_error: "+err_msg);
        throw std::runtime_error(err_msg);
    } else {
        generateLogs_console(logger, "INFO", "genetic_optimizer", "validation_state", "popul_size > 2 : " + std::to_string(popul_size) + " > 2");
    }
    if (P_m < 0 or P_m > 1) {
        err_msg = "P_m < 0 or P_m > 1 : " + std::to_string(P_m) + "< 0 or " + std::to_string(P_m) + " > 1";
        generateLogs_console(logger, "ERROR", "genetic_optimizer", "validation_state", "runtime_error: "+err_msg);
        throw std::runtime_error(err_msg);
    } else {
        generateLogs_console(logger, "INFO", "genetic_optimizer", "validation_state", "0 <= P_m <= 1 : 0 <= " + std::to_string(P_m) + " <= 1");
    }
    if (iter_amount == 0) {
        err_msg = "iter_amount == 0 : " + std::to_string(iter_amount) + " == 0";
        generateLogs_console(logger, "ERROR", "genetic_optimizer", "validation_state", "runtime_error: "+err_msg);
        throw std::runtime_error(err_msg);
    } else {
        generateLogs_console(logger, "INFO", "genetic_optimizer", "validation_state", "iter_amount != 0 : " + std::to_string(iter_amount) + " != 0");
    }

    generateLogs_console(logger, "INFO", "genetic_optimizer", "validation_state", "End validation");

    std::multimap<size_t, std::multimap<IntersectionMetric, GeneticMatrix, std::greater<IntersectionMetric>>> population_per_epoch_map;

    // load matrix
    GeneticMatrix mat = {load_matrix_from_alist(mat_path), "", ""};
    generateLogs_console(logger, "INFO", "genetic_optimizer", "matrix_loading", mat_path+" was loaded succesfully");

    BG_type bg_type;
    auto mat_stem = std::filesystem::path(mat_path).stem();
    if (mat_stem == "BG1") {
        bg_type = BG_type::BG1;
        generateLogs_console(logger, "INFO", "genetic_optimizer", "matrix_loading", "Matrix`s BG_type == BG1");
    } else if (mat_stem == "BG2") {
        bg_type = BG_type::BG2;
        generateLogs_console(logger, "INFO", "genetic_optimizer", "matrix_loading", "Matrix`s BG_type == BG2");
    } else {
        bg_type = BG_type::NOT_5G;
        generateLogs_console(logger, "INFO", "genetic_optimizer", "matrix_loading", "Matrix`s BG_type == NOT_5G");
    }

    // generate population with adding some mutations on expanded matrix
    constexpr IntersectionMetric DEFAULT_KEY = -1.0;

    generateLogs_console(logger, "INFO", "genetic_optimizer", "population_generation", "Start generating poopulation with size "+std::to_string(popul_size));
    std::multimap<IntersectionMetric, GeneticMatrix, std::greater<IntersectionMetric>> population_map;
    population_map.insert({DEFAULT_KEY, {mat.matrix, mat_stem.generic_string(), ""}});
    generateLogs_console(logger, "INFO", "genetic_optimizer", "population_generation", "Origin matrix was inserted into population");
    for (size_t i{1}; i < popul_size; i++) {
        population_map.insert({DEFAULT_KEY, {make_random_mutation(bg_type, mu, get_seed(), mat.matrix), "", ""}});
        generateLogs_console(logger, "INFO", "genetic_optimizer", "population_generation", std::to_string(i)+"th mutated matrix was inserted into population");
    }
    generateLogs_console(logger, "INFO", "genetic_optimizer", "population_generation", "End generating poopulation with size "+std::to_string(popul_size));


    for (size_t epoch{1}; epoch < iter_amount + 1; epoch++) {
        
        size_t elem_index{1};
        for (auto &elem : population_map) {
            elem.second.matrix_id = std::to_string(epoch) + "." + std::to_string(elem_index++);
        }

        // calculate objective function for all individuals
        population_map = calculate_obj_func(bg_type, population_map, QBER_range, Z, logger);

        // print results for epoch
        std::stringstream ss;
        ss << "EPOCH " << epoch << " - best metric: " << population_map.begin()->first << std::endl;
        for (auto individ : population_map) {
            ss << individ.second.matrix_id << " metric " << individ.first << " " << individ.second.history << std::endl;
        }
        generateLogs_csv(csv, "DATA", "genetic_optimizer", "results_printing", ss.str());

        // remember popualtion per epoch
        population_per_epoch_map.insert(std::pair(epoch, population_map));
        generateLogs_console(logger, "INFO", "genetic_optimizer", "results_saved", "Best metric was saved in memory");

        // half best individuals stays, others - throws away
        auto iter_middle = std::next(population_map.begin(), ((population_map.size()+1) / 2));
        population_map.erase(iter_middle, population_map.end());
        generateLogs_console(logger, "INFO", "genetic_optimizer", "population_halfing", "Last half was erased");
        
        // get population values and shuffle it
        std::vector<GeneticMatrix> population_values_vec;
        for (auto iter = population_map.begin(); iter != population_map.end(); iter++) {
            population_values_vec.push_back(iter->second);
        }

        auto rng = std::default_random_engine{};
        rng.seed(get_seed());
        std::ranges::shuffle(population_values_vec, rng);
        // std::cout << "NEXT LINE DEBUG POPULAT SHUFFLE" << std::endl;
        // for (auto elem : population_values_vec) {
        //     std::cout << elem.matrix_id << " ";
        // }
        // std::cout << std::endl;

        // crossover
        for (size_t idx{0}; idx < population_values_vec.size(); idx += 2) {

            size_t next_idx = idx + 1;

            GeneticMatrix first_ind = population_values_vec.at(idx);
            GeneticMatrix second_ind;

            if (next_idx < population_values_vec.size()) {
                second_ind = population_values_vec.at(next_idx);
            } else {
                std::vector<GeneticMatrix> second_ind_vec;
                std::sample(population_values_vec.begin(), population_values_vec.end()-1, std::back_inserter(second_ind_vec), 1, rng);
                second_ind = second_ind_vec.back();
            }

            const auto& [mat_1, mat_2] = crossover(first_ind.matrix, second_ind.matrix);
            
            population_map.insert({DEFAULT_KEY, {mat_1, "(" + first_ind.matrix_id + " x " + second_ind.matrix_id + ")", ""}});
            population_map.insert({DEFAULT_KEY, {mat_2, "(" + second_ind.matrix_id + " x " + first_ind.matrix_id + ")", ""}});
        }
        generateLogs_console(logger, "INFO", "genetic_optimizer", "population_crossovering", "Population was crossovered");

        // add mutations for population
        int seed_inc{1};
        std::multimap<IntersectionMetric, GeneticMatrix, std::greater<IntersectionMetric>> population_map_buf;
        for (auto iter = population_map.begin(); iter != population_map.end(); iter++) {
            if (iter->first != DEFAULT_KEY) { // not crossovered
                iter->second.history = iter->second.matrix_id;
            }

            if (std::uniform_real_distribution<>(0, 1)(rng) < P_m) { // matrix mutates
                GeneticMatrix mat_buf = {make_random_mutation(bg_type, mu, get_seed(), iter->second.matrix), iter->second.history + "m", ""};
                population_map_buf.insert(std::pair(DEFAULT_KEY, mat_buf)); // reset key for changed matrix
            } else { // matrix doesn`t mutates
                population_map_buf.insert(std::pair(iter->first, iter->second));
            }

            seed_inc++;
        }
        generateLogs_console(logger, "INFO", "genetic_optimizer", "population_mutating", "Population was mutated");

        population_map = population_map_buf;
    }

    return population_per_epoch_map;
}

// template std::multimap<size_t, std::multimap<IntersectionMetric, GeneticMatrix, std::greater<IntersectionMetric>>>
// genetic_algo<CSVReceiverT<std::string, std::string, std::string, std::string, std::string>>(size_t popul_size, double P_m, std::string mat_path, size_t Z,
//     std::pair<double, double> QBER_range, size_t mu, size_t iter_amount, const LogSender& logger, const CSVReceiverT<std::string, std::string, std::string, std::string, std::string>& csv
// );
