#include "base-manager.h"

#include <boost/mpi/communicator.hpp>
#include <boost/mpi/environment.hpp>


class Manager: public BaseManager {
public:
	Manager(boost::mpi::communicator const& com);
	auto stop() -> void override;
protected:
	auto _process_task(Task t, ID task_id, ID worker_id) -> void override;
	auto _get_workers_number() const -> size_t override;
	auto _add_processing_thread(Task t, ID task_id, ID worker_id) -> void override;
private:
	boost::mpi::communicator const& m_mpi_communicator;
	std::mutex mpi_receive_mutex;
	std::mutex mpi_send_mutex;
};