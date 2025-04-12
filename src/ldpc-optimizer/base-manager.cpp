#include "base-manager.h"
#include "utils.hpp"

#include <chrono>
#include <sstream>


BaseManager::BaseManager() : m_tasks_semaphore{0},  // Принудительно инициализируем все атрибуты до
					 m_next_id{0},			// инициализации m_workers_semaphore, чтобы 
					 m_results{},			// _get_workers_number не вызывался на
					 m_results_mutex{},		// неинициализированном объекте
					 m_tasks_mutex{},
					 m_workers_mutex{},
					 m_tasks_queue{},
					 m_workers_queue{},
					 m_processing_threads{},
					 m_workers_semaphore{0},
					 m_is_stopped{true}
{}


auto BaseManager::start() -> void
{
	// Отмечаем всех воркеров как свободные
	for (ID worker_id{1}; worker_id <= _get_workers_number(); ++worker_id) {
		m_workers_queue.push(worker_id);
		m_workers_semaphore.release();
	}

	// Запускаем поток-читатель
	std::thread reader_thread{&BaseManager::_run_reader_thread, this};
	reader_thread.detach();

	m_is_stopped = false;
}


auto BaseManager::send_task(Task t) -> ID
{
	if (m_is_stopped) {
		throw std::runtime_error{"Sending tasks to stopped manager is prohibited"};
	}

	std::cout << std::this_thread::get_id() << " BaseManager::send_task: Locking tasks mutex and pushing task to queue...\n";
	std::cout.flush();

	m_tasks_mutex.lock();
	ID task_id{m_next_id++};
	m_tasks_queue.push({task_id, t});
	m_tasks_semaphore.release();

	std::cout << std::this_thread::get_id() << " BaseManager::send_task: Locking results mutex and creating promice...\n";
	std::cout.flush();

	m_results_mutex.lock();
	m_results[task_id] = std::promise<Result>{};

	m_results_mutex.unlock();
	m_tasks_mutex.unlock();

	std::cout << std::this_thread::get_id() << " BaseManager::send_task: Returning task ID...\n";
	std::cout.flush();

	return task_id;
}


auto BaseManager::wait_for_result(ID id) -> Result
{
	try {
		std::cout << std::this_thread::get_id() << " BaseManager::wait_for_result: Waiting for results mutex...\n";
		std::cout.flush();

		m_results_mutex.lock();
		std::future<Result> fut_res{m_results.at(id).get_future()};
		m_results_mutex.unlock();

		std::cout << std::this_thread::get_id() << " BaseManager::wait_for_result: Waiting for result with id " << id << "...\n";
		std::cout.flush();

		fut_res.wait();
		Result res{fut_res.get()};

		std::cout << std::this_thread::get_id() << " BaseManager::wait_for_result: Result received\nErasing result with id " << id << "...\n";
		std::cout.flush();

		m_results_mutex.lock();
		m_results.erase(id);
		m_results_mutex.unlock();

		std::cout << std::this_thread::get_id() << " BaseManager::wait_for_result: ID erased\nBaseManager::wait_for_result: Joining thread...\n";
		std::cout.flush();

		std::lock_guard guard{m_processing_threads_mutex};
		if (m_processing_threads.at(id).joinable()) {
			m_processing_threads.at(id).join();
			m_processing_threads.erase(id);
		}

		std::cout << std::this_thread::get_id() << " BaseManager::wait_for_result: Thread joined and erased\n";
		std::cout.flush();

		return res;
	}
	catch (std::out_of_range exc) {
		std::stringstream err_msg_stream;
		err_msg_stream << "There is no result with ID " << id;
		throw std::out_of_range{err_msg_stream.str()};
	}
}


// Не использовать атрибуты класса в этом методе!!
auto BaseManager::_get_workers_number() const -> size_t
{
	return 10;
}


auto BaseManager::_process_task(Task t, ID task_id, ID worker_id) -> void
{
	// Здесь вместо генерации случайного числа и сна отправляем задачу
	// воркеру с id = worker_id и ждём результата
	size_t sleep_time_seconds{get_rand_pos_int(1, 10)};
	std::this_thread::sleep_for(std::chrono::seconds{sleep_time_seconds});

	// Показываем, что очередной воркер освободился
	m_workers_mutex.lock();
	m_workers_queue.push(worker_id);
	m_workers_mutex.unlock();
	m_workers_semaphore.release();

	std::lock_guard guard{m_results_mutex};
	m_results[task_id].set_value({});
}


auto BaseManager::_run_reader_thread() -> void
{
	while (true) {
		m_workers_semaphore.acquire(); // Ждём свободного воркера
		m_tasks_semaphore.acquire(); // Ждём задачу

		std::lock_guard tasks_guard{m_tasks_mutex};

		auto [task_id, t] = m_tasks_queue.front();
		m_tasks_queue.pop();

		if (task_id == -1) {	// Если пришла задача останова
			break;
		}

		// Получаем id свободного воркера
		m_workers_mutex.lock();
		ID worker_id{m_workers_queue.front()};
		m_workers_queue.pop();
		m_workers_mutex.unlock();

		_add_processing_thread(t, task_id, worker_id);
	}
}


auto BaseManager::stop() -> void
{
	m_is_stopped = true;

	std::lock_guard guard{m_tasks_mutex};
	m_tasks_queue.push({-1, {}}); // Отправляем пустую задачу с ID -1 — задачу останова

	// Ждём всех незаконченных потоков, ждущих результатов от воркеров
	for (auto& processing_thread : m_processing_threads) {
		if (processing_thread.second.joinable()) {
			processing_thread.second.join();
		}
	}
}


auto BaseManager::_add_processing_thread(Task t, ID task_id, ID worker_id) -> void
{
	std::lock_guard threads_guard{m_processing_threads_mutex};
	m_processing_threads.emplace(task_id, std::thread{&BaseManager::_process_task, this, t, task_id, worker_id});
}
