#include <queue>
#include <mutex>
#include <semaphore>


template<typename T>
class sync_queue
{
public:
	sync_queue();
	auto push(T const& val) -> void;
	auto pop() -> T;
private:
	std::mutex m_queue_mutex;
	std::counting_semaphore<> m_queue_semaphore;
	std::queue<T> m_queue;
};


template<typename T>
sync_queue<T>::sync_queue(): m_queue_semaphore{0} {}


template<typename T>
auto sync_queue<T>::push(T const& val) -> void
{
    std::lock_guard guard{m_queue_mutex};
    m_queue.push(val);
    m_queue_semaphore.release();
}


template<typename T>
auto sync_queue<T>::pop() -> T
{
    m_queue_semaphore.acquire();
    std::lock_guard guard{m_queue_mutex};
    T res = m_queue.front();
    m_queue.pop();
    return res;
}