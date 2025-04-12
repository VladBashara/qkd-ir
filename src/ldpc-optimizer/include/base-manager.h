#ifndef BaseManager_H
#define BaseManager_H


#include "task.hpp"
#include "result.h"

#include <queue>
#include <map>
#include <semaphore>
#include <mutex>
#include <thread>
#include <future>


class BaseManager
{
public:
	using ID = long long;
	
	BaseManager();
	auto send_task(Task t) -> ID;
	auto wait_for_result(ID id) -> Result;
	virtual auto stop() -> void;
	virtual auto start() -> void;

protected:
	virtual auto _get_workers_number() const -> size_t;
	virtual auto _process_task(Task t, ID task_id, ID worker_id) -> void;
	virtual auto _run_reader_thread() -> void;
	virtual auto _add_processing_thread(Task t, ID task_id, ID worker_id) -> void;
	
	std::queue<std::pair<ID, Task>> m_tasks_queue;
	std::queue<ID> m_workers_queue;
	std::counting_semaphore<> m_tasks_semaphore;
	std::counting_semaphore<> m_workers_semaphore;
	mutable std::mutex m_workers_mutex;
	mutable std::mutex m_tasks_mutex;
 	mutable std::mutex m_results_mutex;
	mutable std::mutex m_processing_threads_mutex;
	std::map<ID, std::thread> m_processing_threads;
	std::map<ID, std::promise<Result>> m_results;
	ID m_next_id;

private:
	bool m_is_stopped;
};


#endif
