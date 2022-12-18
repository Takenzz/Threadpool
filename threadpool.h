#include <thread>
#include <condition_variable>
#include <queue>
#include <future>
#include <vector>
#include <type_traits>
#include <mutex>
#include <functional>
#include <memory>

class ThreadPool {
    int thread_num = 0;
    std::mutex queue_lock;
    std::queue<std::function<void()>> work_queue;
    std::vector<std::thread> workers;
    std::condition_variable cond;
    bool running;

public:
        ThreadPool(int num = std::thread::hardware_concurrency());
        ~ThreadPool();

        ThreadPool(const ThreadPool &) = delete;
        ThreadPool(ThreadPool &&) = delete;
        ThreadPool &operator=(const ThreadPool &) = delete;
        ThreadPool &operator=(ThreadPool &&) = delete;

    template<class F, class... Args>
            auto submit(F &&f, Args&&... args) ->std::future<typename std::invoke_result<F, Args...>::type>;

private:
    void ThreadRun() {
        while (running) {
            std::function<void()> task;
            {
                std::unique_lock lk(queue_lock);
                cond.wait(lk, [this](){return !this->running || !work_queue.empty();});
                if(!this->running) return;
                task = std::move(work_queue.front());
                work_queue.pop();
            }
            task();
        }
    }
};

ThreadPool::ThreadPool(int num) {
        thread_num = num;
        running = true;
        for(int i = 0; i < num; i++) {
            workers.emplace_back(std::thread(&ThreadPool::ThreadRun, this));
        }
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard lk(queue_lock);
        running = false;
    }
    cond.notify_all();
    for(auto& worker : workers) {
        worker.join();
    }
    workers.clear();
}

template<class F, class... Args>
    auto ThreadPool::submit(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type> {
    std::function<decltype(f(args...))()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
    auto task = std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);
    std::future<decltype(f(args...))> res = task->get_future();
    std::function<void()> wrapper_func = [task](){(*task)();};
    std::lock_guard lk(queue_lock);
    work_queue.emplace(wrapper_func);
    cond.notify_one();
    return res;
}
