#include "threadpool.h"

ThreadPool::ThreadPool(unsigned int threads_cnt)
        :  cnt_threads_(threads_cnt),
        shutdown_(false)
{
    if (threads_cnt <= 1 || threads_cnt >= 10)
    {
        threads_cnt = 2;
        LOG(ERROR) << "线程数量不合法，已初始化为2个线程";
    }

    // 多出来两个线程做其他任务
    for (unsigned i = 0; i < cnt_threads_ + 2; ++ i)
    {
        std::thread t([this](){this->_WorkerRoutine();});
        worker_threads.emplace_back(std::move(t));
    }

    worker_threads.emplace_back(std::thread([this](){this->_MonitorRoutine();}));
}

ThreadPool::~ThreadPool()
{
}

double ThreadPool::load() const
{
    return  GetTaskQueueSize() / GetCurrentSpeed_(); // 至少有两个线程负载垃圾回收等任务
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


            task = std::move(tasks_.front());
            tasks_.pop_front();
        }

        task();

        cnt_tasks_1_sec_ ++;
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

        if (cnt_tasks_1_sec_ == 0 || tasks_.size() == 0)
            continue;

        speeds_.push(cnt_tasks_1_sec_);
        if (speeds_.size() > 3)
            speeds_.pop();

        task_queue_size_.push(tasks_.size() + 2);
        if (task_queue_size_.size() > 3)
            task_queue_size_.pop();

        cnt_tasks_1_sec_ = 0;
    }
}


double ThreadPool::GetCurrentSpeed_() const
{
    std::queue<int> tmp(speeds_);
    int sum = 0;
    unsigned n = tmp.size();
    if (n == 0 || n == 1)
        return 1.0;


    while (!tmp.empty())
    {
        sum += tmp.front();
        tmp.pop();
    }

    double result = 1.0 * sum / n;
    if (result < 1)
        return 1;
    else
        return result;
}

int ThreadPool::GetTaskQueueSize() const
{
    std::queue<int> tmp(task_queue_size_);
    int sum = 0;
    unsigned n = tmp.size();

    while (!tmp.empty())
    {
        sum += tmp.front();
        tmp.pop();
    }

    if (sum == 0)
        return 0;

    return sum / n;
}
