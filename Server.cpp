#include "Server.h"

Server::Server(int cpu, int ram, int id) : cpu_(cpu + 1), storage_(ram), id_(id)
{
    shutdown_ = false;

    // 本地资源管理
//    cpu_.ExecuteTask(std::bind(&Server::GcFunc, this));
}


auto Server::Execute(Task t) -> std::future<bool>
{
    return cpu_.template ExecuteTask([this, t]() -> bool {
        storage_.Malloc(t.storage);

        std::this_thread::sleep_for(std::chrono::milliseconds(t.time));

        storage_.Free(t.storage * 0.2);

        return true;
    });
}

void Server::Stop()
{
    shutdown_ = true;

//    cpu_.Stop();
    cpu_.JoinAll();
}

void Server::PrintStatus()
{
    LOG(INFO) << "server[" << id_ << "] - load:" << GetCpuLoad()
              << ",queue:" << GetTaskQueueSize()
              << ",speed:" << GetCurrentSpeed();
}


void Server::GcFunc()
{
    while (!shutdown_)
    {
        storage_.Free(Config::kGcSize);

        std::this_thread::sleep_for(std::chrono::milliseconds(g_config.GcInterval));
    }
}
