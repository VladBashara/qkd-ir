#include "manager.h"
#include "worker.h"
#include "ldpc-utils.hpp"
#include "file-processor.h"

#include <iostream>
#include <random>


size_t constexpr ITERATIONS{10};
size_t constexpr SHIFTS{10};


int main()
{
	size_t Z{4};

	boost::mpi::environment mpi_environment;
	boost::mpi::communicator mpi_communicator;

	int rank{mpi_communicator.rank()};

	if (rank == 0) { // Manager process
		Eigen::SparseMatrix<GF2, Eigen::RowMajor> bg{load_matrix_from_alist("BG1.alist")};
		size_t m = bg.rows();
		size_t n = bg.cols();

		bg = bg.block(0, 0, m, n);

		Manager manager{mpi_communicator};
		manager.start();

		std::random_device r;
		std::mt19937 rand_gen(r());

		std::uniform_int_distribution inverse_row_distribution{0, 45};
		std::uniform_int_distribution inverse_col_distribution{0, 26};

		for (size_t iter_number{0}; iter_number < ITERATIONS; ++iter_number) {

			std::cout << "Iteration " << iter_number << std::endl;
			std::cout.flush();		

			Eigen::SparseMatrix<GF2, Eigen::RowMajor> H = enhance_from_base(bg, Z);

			size_t row_to_inverse{inverse_row_distribution(rand_gen)};
			size_t col_to_inverse{inverse_col_distribution(rand_gen)};

			// Инвертируем один бит в случайной позиции
			H.coeffRef(row_to_inverse, col_to_inverse) = !H.coeffRef(row_to_inverse, col_to_inverse);

			std::vector<BaseManager::ID> task_ids;
			for (size_t shift_number{0}; shift_number < SHIFTS; ++shift_number) {
				H = shift_eyes(H, Z, BG_type::NOT_5G, shift_randomness::COMBINE); // check if COMBINE is a right choice
				Task t{0.0, 0.11, 0.01, LDPC_algo::NMS, false, H};
				task_ids.push_back(manager.send_task(t));
			}

			std::cout << "Tasks created, waiting for results..." << std::endl;
			std::cout.flush();	

			std::vector<Result> results;
			for (BaseManager::ID task_id : task_ids) {
				results.push_back(manager.wait_for_result(task_id));
			}

			std::cout << "Results ready" << std::endl;
			std::cout.flush();	

			Result av_result{compute_average_result(results)};

			std::cout << av_result << std::endl;
		}

		manager.stop();
	}
	else { // Worker process
		Worker w{mpi_communicator};
		w.run();
	}

	return 0;
}