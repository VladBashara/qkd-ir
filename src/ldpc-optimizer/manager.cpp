#include "manager.h"


Manager::Manager(boost::mpi::communicator const& com) : BaseManager{}, m_mpi_communicator{com} {}


auto Manager::_process_task(Task t, ID task_id, ID worker_id) -> void
{
	std::cout << std::this_thread::get_id() << " Manager::_process_task: Sending task " << task_id << " to worker with id " << worker_id << "...\n";
	std::cout.flush();

	mpi_send_mutex.lock();
	m_mpi_communicator.send(worker_id, 0, t);
	mpi_send_mutex.unlock();

	std::cout << std::this_thread::get_id() << " Manager::_process_task: Waiting for result of task " << task_id << " from worker with id " << worker_id << "...\n";
	std::cout.flush();

	Result result;
	mpi_receive_mutex.lock();
	m_mpi_communicator.probe(worker_id, boost::mpi::any_tag);
	boost::mpi::status receive_status{m_mpi_communicator.recv(worker_id, boost::mpi::any_tag, result)};
	mpi_receive_mutex.unlock();
	// if (receive_status.error()) {
	// 	std::stringstream err_msg_stream;
	// 	err_msg_stream << "Boost receive error happened while receiving result from worker #" << worker_id << " code: " << receive_status.error();
	// 	throw std::runtime_error{err_msg_stream.str()}; // Заменить на установку исключения в promice
	// }

	// Показываем, что очередной воркер освободился

	std::cout << std::this_thread::get_id() << " Manager::_process_task: Locking workers mutex...\n";
	std::cout.flush();

	m_workers_mutex.lock();

	std::cout << std::this_thread::get_id() << " Manager::_process_task: Pushing worker id " << worker_id << "...\n";
	std::cout.flush();

	m_workers_queue.push(worker_id);

	std::cout << std::this_thread::get_id() << " Manager::_process_task: Unlocking workers mutex...\n";
	std::cout.flush();

	m_workers_mutex.unlock();

	std::cout << std::this_thread::get_id() << " Manager::_process_task: Releasing workers semaphore...\n";
	std::cout.flush();

	m_workers_semaphore.release();

	std::cout << std::this_thread::get_id() << " Manager::_process_task: Locking results mutex...\n";
	std::cout.flush();

	std::lock_guard guard{m_results_mutex};

	std::cout << std::this_thread::get_id() << " Manager::_process_task: setting value of result for task #" << task_id << "...\n";
	std::cout.flush();

	m_results[task_id].set_value(result);

	std::cout << std::this_thread::get_id() << " Manager::_process_task: value is set\n";
	std::cout.flush();
}


auto Manager::stop() -> void {
	BaseManager::stop();

	// Копируем очередь со всеми воркерами
	m_workers_mutex.lock();
	std::queue<BaseManager::ID> workers_queue{m_workers_queue};
	m_workers_mutex.unlock();

	while ( !workers_queue.empty() ) {
		BaseManager::ID worker_id = workers_queue.front();
		workers_queue.pop();
		
		m_mpi_communicator.send(worker_id, 0, Task{});
		
		m_workers_mutex.lock();
		m_workers_queue.pop();
		m_workers_mutex.unlock();
	}
}


// Не использовать атрибуты класса в этом методе!!
auto Manager::_get_workers_number() const -> size_t
{
	return m_mpi_communicator.size() - 1;
}


auto Manager::_add_processing_thread(Task t, ID task_id, ID worker_id) -> void
{
	std::lock_guard threads_guard{m_processing_threads_mutex};
	m_processing_threads.emplace(task_id, std::thread{&Manager::_process_task, this, t, task_id, worker_id});
}