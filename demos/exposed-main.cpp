#include "ldpc-utils.hpp"
#include "file-processor.h"
#include "result-exposed.h"

#include <iostream>
#include <random>
#include <taskflow/taskflow.hpp>


size_t constexpr ITERATIONS{50};
size_t constexpr EPOCHS{3};
size_t constexpr SHIFTS{10};
size_t constexpr Z{4};


void print_matrix_as_triplets(Eigen::SparseMatrix<GF2, Eigen::RowMajor> const &matrix) {
    std::cout << "ROW\tCOL\tVAL\n";
    for (int k = 0; k < matrix.outerSize(); k++) {
        for (Eigen::SparseMatrix<GF2, Eigen::RowMajor>::InnerIterator it(matrix, k); it; ++it) {
            std::cout << it.row() << "\t" << it.col() << "\t" << it.value() << std::endl;
        }
    }
}


int main() {
    Eigen::SparseMatrix<GF2, Eigen::RowMajor> bg{load_matrix_from_alist("BG1.alist")};
    size_t m = bg.rows();
    size_t n = bg.cols();

    bg = bg.block(0, 0, 22, 44); // R = 1/2

    std::random_device r;
    std::mt19937 rand_gen(r());

    std::uniform_int_distribution inverse_row_distribution{0, 21};
    std::uniform_int_distribution inverse_col_distribution{0, 26};

    Eigen::SparseMatrix<GF2, Eigen::RowMajor> H = enhance_from_base(bg, Z);
    H.makeCompressed();

    // Вычисление начального значения
    Eigen::SparseMatrix<GF2, Eigen::RowMajor> shifted_H = shift_eyes(H, Z, BG_type::BG1, shift_randomness::NO_RANDOM);
    shifted_H.makeCompressed();
    benchmarks::ExposedBUSChannellWynersEC busc_bm("BG1.alist", BG_type::BG1, 22, 44, 4);
    benchmarks::BenchmarkRange ber_range(0.01, 0.1, 0.01);
    benchmarks::BenchmarkRange exposed_range(0.01, 0.99, 0.01);
    int iteration = 0;
    for (int i = 0; i < ber_range.length; ++i) {
        for (int j = 0; j < exposed_range.length; ++j) {
            ++iteration;
            std::cout << std::to_string(ber_range.start + i * ber_range.step) << ":";
            std::cout << std::to_string(exposed_range.start + j * exposed_range.step) << " -> ";
            std::cout << std::to_string(iteration) << std::endl;
        }
    }


    std::cout << std::to_string(exposed_range.length * ber_range.length) << std::endl;

    for (int i = 0; i < ber_range.length; ++i) {
        double current_ber = ber_range.start + i * ber_range.step;
        benchmarks::BenchmarkRange current_ber_range(current_ber, current_ber, 0.01);

        ExposedResult current_res{busc_bm.run(current_ber_range, exposed_range, LDPC_algo::SP, true)};
        std::cout << current_res << std::endl;
        std::string csv_out = exported_result_to_csv(current_res);

        for (char & k : csv_out){
            if(k == '.'){
                k = ',';
            }
        }

        std::ofstream of("./results/tmp/results"+ std::to_string(current_ber) + ".csv");
        if (of) {
            of << csv_out;
            of.close();
        } else {
            std::cerr << "can't open output file" << std::endl;
        }
    }



    return 0;
//    try {
//        for (size_t epoch_number{1}; epoch_number <= EPOCHS; ++epoch_number) {
//            std::cout << "Epoch" << epoch_number << std::endl;
//            std::cout.flush();
//
//            for (size_t iter_number{0}; iter_number < ITERATIONS; ++iter_number) {
//
//                std::cout << "Iteration " << iter_number << std::endl;
//                std::cout.flush();
//
//                Eigen::SparseMatrix<GF2, Eigen::RowMajor> bg_local{bg};
//
//                for (size_t inv_number{0}; inv_number < epoch_number; ++inv_number) {
//                    size_t row_to_inverse{inverse_row_distribution(rand_gen)};
//                    size_t col_to_inverse{inverse_col_distribution(rand_gen)};
//
//                    // Инвертируем EPOCHS бит в случайной позиции
//                    bg_local.coeffRef(row_to_inverse, col_to_inverse) = !bg_local.coeffRef(row_to_inverse,
//                                                                                           col_to_inverse);
//                }
//                bg_local.makeCompressed();
//
//                Eigen::SparseMatrix<GF2, Eigen::RowMajor> H_local = enhance_from_base(bg_local, Z);
//                H_local.makeCompressed();
//
//                std::vector<ExposedResult> results;
//                results.reserve(SHIFTS);
//
//                tf::Executor executor;
//                tf::Taskflow taskflow;
//                std::mutex results_sync;
//
//                for (size_t shift_number{0}; shift_number < SHIFTS; ++shift_number) {
//                    taskflow.emplace([&, shift_number]() {
//
//                        Eigen::SparseMatrix<GF2, Eigen::RowMajor> shifted_H_local = shift_eyes(H_local, Z, BG_type::BG1,
//                                                                                               shift_randomness::COMBINE);
//                        shifted_H_local.makeCompressed();
//                        benchmarks::ExposedBUSChannellWynersEC busc_bm{shifted_H_local};
//
//                        ExposedResult res{busc_bm.run(ber_range, exposed_range, LDPC_algo::NMS, true)};
//
//                        results_sync.lock();
//                        results.push_back(res);
//                        results_sync.unlock();
//                    });
//                }
//
//                executor.run(taskflow).wait();
//
//                ExposedResult av_result{compute_average_result(results)};
//
//                std::cout << av_result << std::endl;
//
//                if (av_result < current_res) {
//                    std::cout << "Better matrix found\n";
//                    print_matrix_as_triplets(bg_local);
//                    std::cout.flush();
//                    bg = bg_local;
//                    current_res = av_result;
//                }
//            } // for (size_t iter_number{0}; iter_number < ITERATIONS; ++iter_number)
//        } // for (size_t epoch_number{0}; epoch_number < EPOCHS; ++epoch_number)
//    } // try
//
//
//    catch (std::runtime_error err) {
//        std::cerr << "Exception occured: " << err.what() << std::endl;
//    }
//

    return 0;
}