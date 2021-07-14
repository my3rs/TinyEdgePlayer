#include "threadpool.h"
#include "config.h"

#include <numeric>

ThreadPool::ThreadPool(unsigned int threads_cnt)
        :  cnt_threads_(threads_cnt),
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

            int time_dur = (clock() - task_enter_time_.front()) / CLOCKS_PER_SEC * 1000;
            task_enter_time_.pop_front();

            if (time_dur >= Config::kLatencyThreshold)
            {
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

        /* 1. load */
        loads_.push_back(GetLoad());
        while (loads_.size() > 3)
            loads_.pop_front();

        /* 2. 阻塞的任务数量 */
        blocked_tasks_.push_back(blocked_tasks_in_one_second_);
        blocked_tasks_in_one_second_ = 0;
        while (blocked_tasks_.size() > 3)
            blocked_tasks_.pop_front();

        /* 3. speed 和 task queue size */

        // 后面这个if内的代码是为了保证
        // “线程处理速度”和“任务队列长度”两个指标的值不为0
        if (tasks_completed_in_one_second_ == 0 || tasks_.size() == 0)
        {
            speeds_.push_back(1);
            task_queue_size_.push_back(1);

            // 不用判断speeds_和task_queue_size_的长度是不是大于3
            // 直接进入到下一轮多pop一次就好

            continue;   // 一定要跳过本次循环
        }
            

        speeds_.push_back(tasks_completed_in_one_second_);
        while (speeds_.size() > 3)  // 别用if
            speeds_.pop_front();

        task_queue_size_.push_back(tasks_.size() + 2);
        while (task_queue_size_.size() > 3) // 别用if
            task_queue_size_.pop_front();

        tasks_completed_in_one_second_ = 0;
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
    // 所以该函数不应该频繁被调用？
    power_ = 0.37 * power_ + 0.63 * (GetCurrentSpeed() / GetTaskQueueSize());   // 指数移动平均
    return power_;
}


double ThreadPool::GetBlockRate()
{
    std::deque<unsigned> tmp(blocked_tasks_);

    unsigned sum = std::accumulate(tmp.begin(), tmp.end(), 0.0);
    double mean = sum / tmp.size();

    return mean / GetCurrentSpeed();
}

unsigned ThreadPool::GetThreadCount()
{
    return worker_threads.size();
}