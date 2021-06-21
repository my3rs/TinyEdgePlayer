#ifndef SEAHI_THREADPOOL_H
#define SEAHI_THREADPOOL_H

#include <atomic>
#include <deque>
#include <mutex>
#include <functional>
#include <future>
#include <queue>
#include <glog/logging.h>


class ThreadPool
{
public:
    explicit ThreadPool(unsigned thread_count);
    ~ThreadPool();

    ThreadPool() = delete;
    ThreadPool(const ThreadPool&) = delete;
    void operator=(const ThreadPool&) = delete;

    void Stop()
    {
        shutdown_ = true;
    }

    double  load() const;

    /**
     * 等待所有线程结束，然后关闭线程池
     */
    void JoinAll();

    /**
     * 向线程池中添加一个任务
     */
    template<typename F, typename... Args>
    auto ExecuteTask(F&& f, Args&&... args)
        -> std::future<typename std::result_of<F (Args...)>::type>;

    /* 
     * get 最近三个周期内的处理速度平均值 
     */
    double GetCurrentSpeed_() const;

    /*
     * get 最近三个周期内的等待队列平均长度 
     */
    int GetTaskQueueSize() const;

private:
    /**
     * worker 线程函数
     */
    void _WorkerRoutine();

    /* 
     * monitor 线程函数
     */
    void _MonitorRoutine();


private:
    unsigned                cnt_threads_;
    std::deque<std::thread> worker_threads;             // 线程池中的线程


    std::mutex              mutex_;
    std::condition_variable cond_;
    bool                    shutdown_;
    std::deque<std::function<void ()>>  tasks_; // 任务队列

    std::atomic<unsigned>   tasks_completed_in_one_second_;

    std::queue<int>         speeds_;
    std::queue<int>         task_queue_size_;
};

template<typename F, typename... Args>
auto ThreadPool::ExecuteTask(F &&f, Args&&... args)
            -> std::future<typename std::result_of<F (Args...)>::type>
{
    using result_type = typename std::result_of<F (Args...)>::type;

    std::unique_lock<std::mutex> guard(mutex_);

    if (shutdown_)
        return std::future<result_type>();

    auto promise = std::make_shared<std::promise<result_type>>();

    auto future = promise->get_future();

    auto func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
    auto task = [t = std::move(func), pm = promise]() mutable {
        try {
            if constexpr (std::is_same<void, result_type>::value) {
                t();
                pm->set_value();
            } else {
                pm->set_value(t());
            }
        } catch (...) {
            pm->set_exception(std::current_exception());
        }
    };

    tasks_.emplace_back(std::move(task));
//    LOG(INFO) << "ThreadPool task size: " << tasks_.size();

    cond_.notify_one();

    return future;
}

#endif //SEAHI_THREADPOOL_H
