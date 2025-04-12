#include "worker.h"

#include <iostream>


void Worker::run() {
	std::cout << "Worker " << m_mpi_rank << ": starting..." << std::endl;
	std::cout.flush();
	
	while (m_is_running) {
		Task task;
		boost::mpi::status status;
		
		// Получаем задачу

		std::cout << "Worker " << m_mpi_rank << ": waiting for task..." << std::endl;
		std::cout.flush();

		status = m_mpi_communicator.recv(0, boost::mpi::any_tag, task);
		
		std::cout << "Worker " << m_mpi_rank << ": task received..." << std::endl;
		std::cout.flush();

		// Проверяем сигнал остановки используя новый метод
		if (_is_final_task(task)) {
			m_is_running = false;
			break;
		}

		std::cout << "Worker " << m_mpi_rank << ": processing task..." << std::endl;
		std::cout.flush();

		Result result = _process_task(task);

		std::cout << "Worker " << m_mpi_rank << ": Task processed, sending result..." << std::endl;
		std::cout.flush();
		
		m_mpi_communicator.send(0, 0, result);
		std::cout << "Worker " << m_mpi_rank << ": Result sent" << std::endl;
		std::cout.flush();
	}
	
	std::cout << "Worker " << m_mpi_rank << " finished" << std::endl;
}

Result Worker::_process_task(Task task) {
	benchmarks::BUSChannellWynersEC busc_bm{task.get_H(), {0.01, 0.02, 0.04, 0.08}, {-1, -1}, true};
	return busc_bm.run(task.ber_start, task.ber_stop, task.ber_step, task.alg_type, task.verbose);
}

bool Worker::_is_final_task(Task& t) {
	return t.is_work_complete();
}