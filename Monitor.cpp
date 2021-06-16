#include "Monitor.h"

Monitor::Monitor()
    : shutdown_(false)
{

}

void Monitor::Stop()
{
    shutdown_ = true;

    if (monitor_thread_.joinable())
        monitor_thread_.join();
}

Monitor & Monitor::Instance()
{
    static Monitor m;
    return m;
}

void Monitor::Init(const std::vector<std::shared_ptr<Server>> &server_pool)
{
    servers_ = server_pool;

    monitor_thread_ = std::thread([this] { ThreadFunc(); });
}

void Monitor::ThreadFunc()
{
    while (!shutdown_)
    {
        for (const auto& server : servers_)
        {
            server->PrintStatus();
        }

        if (shutdown_)       // 如果执行PrintStatus()操作后，shutdown变化，可以立即跳出，无需等待sleep
            return;

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}