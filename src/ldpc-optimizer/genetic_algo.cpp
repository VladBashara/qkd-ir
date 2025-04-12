#include "ldpc-utils.hpp"
#include "file-processor.h"
#include "genetic_algo_utils.hpp"
#include "genetic_algo.hpp"
#include "result-mt.h"

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

size_t get_seed() {
    return std::chrono::steady_clock::now().time_since_epoch().count();
}

bool is_default_key(Result res) {
    auto fers = res.result.fers;
    auto fer_std_devs = res.result.fer_std_devs;

    auto it_unique_fers = std::unique(fers.begin(), fers.end());
    auto it_unique_fer_std_devs = std::unique(fer_std_devs.begin(), fer_std_devs.end());

    fers.resize(std::distance(fers.begin(), it_unique_fers));
    fer_std_devs.resize(std::distance(fer_std_devs.begin(), it_unique_fer_std_devs));

    if (fers.size() == 1 and fers.back() == -1.0 and fer_std_devs.size() == 1 and fer_std_devs.back() == -1.0) {
        return true;
    }
    return false;
}

Result make_default_key(std::pair<double, double> QBER_range, double QBER_step) {
    Result result;

    const size_t vec_size = ((QBER_range.second - QBER_range.first) / QBER_step) + 1;

    result.result.bers = std::vector<double>(vec_size);

    for (size_t i{0} ; i < result.result.bers.size(); i++) {
        result.result.bers.at(i) = QBER_range.first + i*QBER_step;
    }

    result.result.fers = std::vector<double>(vec_size, -1.0);
    result.result.fer_std_devs = std::vector<double>(vec_size, -1.0);

    return result;
}

void print_result_of_genetic_algo(const std::multimap<size_t, std::multimap<Result, GeneticMatrix, std::greater<Result>>> &obj_func_map) {

    std::stringstream sstream;
    for (auto obj_func : obj_func_map) {
        size_t iter = obj_func.first;
        double metric = obj_func.second.begin()->first.intersection_metric;

        sstream << "EPOCH " << iter << " - best metric: " << metric << std::endl;

        for (auto it=obj_func.second.begin(); it!=obj_func.second.end(); it++) {
            sstream << it->second.matrix_id << " metric " << it->first.intersection_metric << " " << it->second.history << std::endl;
        }

        sstream << std::endl;
    }
    
    std::cout << sstream.str();
}

std::multimap<Result, GeneticMatrix, std::greater<Result>> calculate_obj_func(
    const BG_type bg_type, const std::multimap<Result, GeneticMatrix, std::greater<Result>> &population_map, const std::pair<double, double> QBER_range,
    const double QBER_step, const size_t Z) {

    std::multimap<Result, GeneticMatrix, std::greater<Result>> results_map;

    Result DEFAULT_KEY = make_default_key(QBER_range, QBER_step);
    
    tf::Executor executor;
    tf::Taskflow taskflow;
    std::mutex results_sync;
    
    for (auto &[res, gen_matrix] : population_map) { 

        if ( !is_default_key(res) ) { // skip already calculated values
            results_map.insert(std::pair(res, gen_matrix));
            continue;
        }

        taskflow.emplace([&](){

            GeneticMatrix expanded_mat = {enhance_from_base(gen_matrix.matrix, Z), gen_matrix.history, gen_matrix.matrix_id};
            expanded_mat = {shift_eyes(expanded_mat.matrix, Z, bg_type, shift_randomness::COMBINE), gen_matrix.history, gen_matrix.matrix_id};
            expanded_mat.matrix.makeCompressed();

            benchmarks::BSChannellWynersEC busc_bm{expanded_mat.matrix};

            Result obj_func{busc_bm.run(QBER_range.first, QBER_range.second, QBER_step, LDPC_algo::NMS, false)};

            results_sync.lock();
            results_map.insert(std::pair(obj_func, gen_matrix));
            results_sync.unlock();
        });
    }

    executor.run(taskflow).wait();

    return results_map;
}

std::multimap<size_t, std::multimap<Result, GeneticMatrix, std::greater<Result>>> genetic_algo(const size_t popul_size, const double P_m, const std::string mat_path, const size_t Z,
                  const std::pair<double, double> QBER_range, const double QBER_step, const size_t mu, const size_t iter_amount) {
    
    if (popul_size <= 2) {
        throw std::runtime_error("popul_size <= 2 : " + std::to_string(popul_size));
    }
    if (P_m < 0 or P_m > 1) {
        throw std::runtime_error("P_m < 0 or P_m > 1 : " + std::to_string(P_m));
    }
    if (iter_amount == 0) {
        throw std::runtime_error("iter_amount == 0");
    }


    std::multimap<size_t, std::multimap<Result, GeneticMatrix, std::greater<Result>>> population_per_epoch_map;

    // load matrix
    GeneticMatrix mat = {load_matrix_from_alist(mat_path), "", ""};

    BG_type bg_type;
    auto mat_stem = std::filesystem::path(mat_path).stem();
    if (mat_stem == "BG1") {
        bg_type = BG_type::BG1;
    } else if (mat_stem == "BG2") {
        bg_type = BG_type::BG2;
    } else {
        bg_type = BG_type::NOT_5G;
    }

    // generate population with adding some mutations on expanded matrix
    Result DEFAULT_KEY = make_default_key(QBER_range, QBER_step);

    std::multimap<Result, GeneticMatrix, std::greater<Result>> population_map;
    population_map.insert({DEFAULT_KEY, {mat.matrix, mat_stem.generic_string(), ""}});
    for (size_t i{1}; i < popul_size; i++) {
        population_map.insert({DEFAULT_KEY, {make_random_mutation(bg_type, mu, get_seed(), mat.matrix), "", ""}});
    }


    for (size_t epoch{1}; epoch < iter_amount + 1; epoch++) {

        // calculate objective function for all individuals
        population_map = calculate_obj_func(bg_type, population_map, QBER_range, QBER_step, Z);

        size_t elem_index{1};
        for (auto &elem : population_map) {
            elem.second.matrix_id = std::to_string(epoch) + "." + std::to_string(elem_index++);
        }

        // print results for epoch
        std::cout << "EPOCH " << epoch << " - best metric: " << population_map.begin()->first.intersection_metric << std::endl;
        for (auto individ : population_map) {
            std::cout << individ.second.matrix_id << " metric " << individ.first.intersection_metric << " " << individ.second.history << std::endl;
        }
        std::cout << std::endl;
        std::cout.flush();

        // remember popualtion per epoch
        population_per_epoch_map.insert(std::pair(epoch, population_map));

        // half best individuals stays, others - throws away
        auto iter_middle = std::next(population_map.begin(), ((population_map.size()+1) / 2));
        population_map.erase(iter_middle, population_map.end());
        
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

        // add mutations for population
        int seed_inc{1};
        std::multimap<Result, GeneticMatrix, std::greater<Result>> population_map_buf;
        for (auto iter = population_map.begin(); iter != population_map.end(); iter++) {
            if ( !is_default_key(iter->first) ) { // not crossovered
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

        population_map = population_map_buf;
    }

    return population_per_epoch_map;
}