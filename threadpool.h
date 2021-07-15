#ifndef SEAHI_THREADPOOL_H
#define SEAHI_THREADPOOL_H

#include <atomic>
#include <deque>
#include <mutex>
#include <functional>
#include <future>
#include <queue>
#include <ctime>

#include <glog/logging.h>

/*
* 线程池
* 用来模拟CPU，线程数量固定不可扩展，因为每一个线程代表一个CPU核心
* ExecuteTask() 函数用来接受一个任务，任务类型为一个可执行对象
*/

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


    /* get 瞬时使用率 */
    double  GetLoad() const;

    /* get 平均使用率 */
    double  GetAverageLoad() const;

    /* get 算力 */
    double  GetPower();


    /* 等待所有线程结束，然后关闭线程池 */
    void JoinAll();

    /* 向线程池中添加一个任务 */
    template<typename F, typename... Args>
    auto ExecuteTask(F&& f, Args&&... args)
        -> std::future<typename std::result_of<F (Args...)>::type>;

    /* get 最近三个周期内的处理速度平均值 */
    double GetCurrentSpeed() const;

    /* get 最近三个周期内的等待队列平均长度 */
    int GetTaskQueueSize() const;

    /* get 阻塞率（平均值） */
    double GetBlockRate();

    /* get 线程数量 */
    unsigned GetThreadCount();

    /* set 平均任务耗时 */
    void SetAvgTaskTime(double t);

private:
    /* worker 线程函数 */
    void _WorkerRoutine();

    /* monitor 线程函数 */
    void _MonitorRoutine();


private:
    unsigned                cnt_threads_;
    std::deque<std::thread> worker_threads;             // 线程池中的线程


    std::mutex              mutex_;
    std::condition_variable cond_;
    bool                    shutdown_;

    /* 以下两个队列要同时操作进出 */
    std::deque<std::function<void ()>>  tasks_; // 任务队列
    std::deque<std::chrono::system_clock::time_point>     task_enter_time_;   // 每一个任务的入队时间

    std::atomic<unsigned>   tasks_completed_in_one_second_;     // 1秒内完成的任务数量，每秒清除一次
    std::atomic<unsigned>   blocked_tasks_in_one_second_;       // 没有在规定时间内完成的任务数量，每秒清除一次
    std::deque<unsigned>    blocked_tasks_;     // 最近3次的阻塞任务数量
    std::deque<unsigned>    speeds_;            // 最近3次的任务处理速度
    std::deque<unsigned>    task_queue_size_;   // 最近3次的队列长度
    std::deque<double>      loads_;             // 最近3次的线程池使用率

    double                  power_;             // 算力，初始值为线程池中的线程数量

    double                  avg_task_time_;     // 平均任务耗时，由 Server 调用 SetAvgTaskTime(double) 接口进行设置，初始为50
    
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

    // 任务入队的同时进行计时
    task_enter_time_.emplace_back(std::chrono::system_clock::now());
    tasks_.emplace_back(std::move(task));
//    LOG(INFO) << "ThreadPool task size: " << tasks_.size();

    cond_.notify_one();

    return future;
}

#endif //SEAHI_THREADPOOL_H
