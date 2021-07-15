#include "threadpool.h"
#include "config.h"

#include <numeric>

ThreadPool::ThreadPool(unsigned int threads_cnt)
        :  cnt_threads_(threads_cnt),
        avg_task_time_(50),
        shutdown_(false),
        power_(threads_cnt)
{
    // 线程数量最少为2
    // 一个执行MonitorRoutine()，一个执行WorkerRoutine()
    if (threads_cnt <= 1 || threads_cnt >= 10)
    {
        threads_cnt = 2;
        LOG(ERROR) << "线程数量不合法，已初始化为2个线程";
    }

    // 多出来一个线程作为monitor
    for (unsigned i = 0; i < cnt_threads_ + 1; ++ i)
    {
        std::thread t([this](){this->_WorkerRoutine();});
        worker_threads.emplace_back(std::move(t));
    }

    worker_threads.emplace_back(std::thread([this](){this->_MonitorRoutine();}));
}

ThreadPool::~ThreadPool()
{
}

double ThreadPool::GetLoad() const
{
    return  GetTaskQueueSize() / GetCurrentSpeed(); 
}


void ThreadPool::JoinAll()
{
    decltype(worker_threads) tmp;

    {
        std::unique_lock<std::mutex> guard(mutex_);

        if (shutdown_)
            return;

        shutdown_ = true;
        cond_.notify_all();

        tmp.swap(worker_threads);
    }

    for (auto& t : tmp)
    {
        if (t.joinable())
            t.join();
    }

}

void ThreadPool::_WorkerRoutine()
{
    while (true)
    {
        std::function<void ()> task;

        {
            std::unique_lock<std::mutex> guard(mutex_);

            cond_.wait(guard, [this](){
                return shutdown_ || !tasks_.empty();
            });


            if (shutdown_ && tasks_.empty())
            {   // 仅当 shutdown_ 为 true 且任务队列为空时
                // 当前线程结束 routine
                return;
            }

            // 从任务队列移除一个任务的同时，必须同步移除一个 task_enter_time
            task = std::move(tasks_.front());
            tasks_.pop_front();

            auto start = task_enter_time_.front();
            task_enter_time_.pop_front();

            auto time_dur = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start);

            if (time_dur.count() >= avg_task_time_ * 0.8)
            {   // 如果任务的等待时间超过任务平均耗时的80%，认为该任务阻塞时间过长，标记为 blocked_task
                blocked_tasks_in_one_second_++;
            }
        }

        task();

        tasks_completed_in_one_second_ ++;
    }
}

// 每隔一秒计算一下：上一秒处理的任务数量，即速度
void ThreadPool::_MonitorRoutine()
{
    while (true)
    {
        if (shutdown_ && tasks_.empty())
        {
            return;
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));

        /* 1. CPU 使用率 */
        loads_.push_back(GetLoad());
        while (loads_.size() > 3)
            loads_.pop_front();

        /* 2. 阻塞的任务数量 */
        blocked_tasks_.push_back(blocked_tasks_in_one_second_);
        blocked_tasks_in_one_second_ = 0;
        while (blocked_tasks_.size() > 3)
            blocked_tasks_.pop_front();

        /* 3. 任务处理速度 */
        if (tasks_completed_in_one_second_ == 0)
        {
            speeds_.push_back(1);
        }
        else
        {
            speeds_.push_back(tasks_completed_in_one_second_);
            tasks_completed_in_one_second_ = 0;
        }
        while (speeds_.size() > 3)  // 别用if
            speeds_.pop_front();

        /* 4. 任务队列长度 */
        if (tasks_.empty())
        {
            task_queue_size_.push_back(1);
        }
        else
        {
            task_queue_size_.push_back(tasks_.size() + 2);
        }

        while (task_queue_size_.size() > 3) // 别用if
            task_queue_size_.pop_front();
    }
}


double ThreadPool::GetCurrentSpeed() const
{
    std::deque<unsigned> tmp(speeds_);

    if (tmp.empty())
        return 1.0;

    unsigned sum = std::accumulate(tmp.begin(), tmp.end(), 0.0);
    double mean = sum / tmp.size();

    if (mean < 1)
    {   // 这个 if 应该永远不成立，因为 speeds_ 中不会有小于 1 的值存在
        LOG(ERROR) << "Threadpool: speed < 1";
        return 1.0;
    }
    else
    {
        return mean;
    }
}

int ThreadPool::GetTaskQueueSize() const
{
    std::deque<unsigned> tmp(task_queue_size_);

    if (tmp.empty())
        return 0;

    unsigned sum = std::accumulate(tmp.begin(), tmp.end(), 0.0);
    double mean = sum / tmp.size();

    return mean;
}


double ThreadPool::GetAverageLoad() const
{
    std::deque<double> tmp(loads_);

    if (tmp.empty())
        return 0;

    double sum = std::accumulate(tmp.begin(), tmp.end(), 0.0);
    double mean = sum / tmp.size();

    return mean;
}

double ThreadPool::GetPower()
{
    // 每次调用都会计算新值
    power_ = 0.37 * power_ + 0.63 * (GetCurrentSpeed() / GetTaskQueueSize());   // 指数移动平均
    return power_;
}


double ThreadPool::GetBlockRate()
{
    std::deque<unsigned> tmp(blocked_tasks_);

    if (tmp.empty())
        return 0.01;

    unsigned sum = std::accumulate(tmp.begin(), tmp.end(), 0.0);
    double mean = sum / tmp.size();

    double rate = mean / GetCurrentSpeed();

    return rate < 0.01 ? 0.01 : rate;
}

unsigned ThreadPool::GetThreadCount()
{
    return worker_threads.size();
}


void ThreadPool::SetAvgTaskTime(double t)
{
    avg_task_time_ = t;
}