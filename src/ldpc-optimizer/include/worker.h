#ifndef WORKER_H
#define WORKER_H

#include <boost/mpi.hpp>
#include "task.hpp"
#include "result.h"

class Worker {
public:
	explicit Worker(boost::mpi::communicator& com) // для явного вызова, можно поменять при следующем коммите
		: m_mpi_communicator{com} // список инициализации, который инициализирует член класса
		, m_is_running{true}
	{
		m_mpi_rank = m_mpi_communicator.rank();
	}
	void run();

private:
	boost::mpi::communicator& m_mpi_communicator;
	int m_mpi_rank;
	bool m_is_running;

	Result _process_task(Task task);
	bool _is_final_task(Task& t);
};


#endif