#include "Server.h"

Server::Server(int cpu, int ram, int id)
    : cpu_(cpu + 1),
        storage_(ram), id_(id),
        rate_limiter_(Config::kDefaultRateLimit)
{
    shutdown_ = false;

    // 本地资源管理
    cpu_.ExecuteTask(std::bind(&Server::GcFunc, this));
}


auto Server::Execute(Task t) -> std::future<bool>
{
    rate_limiter_.pass();

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

std::string Server::GetStatusLogString()
{
    std::string ret;
    ret += "server[" + std::to_string(id_) + "] - load:"  + std::to_string(GetCpuLoad())
                + ",queue:" + std::to_string(GetTaskQueueSize())
                + ",speed:" + std::to_string(GetCurrentSpeed());
    return ret;
}

void Server::PrintStatus()
{
    LOG(INFO) << GetStatusLogString();
}


void Server::GcFunc()
{
    while (!shutdown_)
    {
        storage_.Free(Config::kGcSize);

        if (shutdown_)
            return;

        std::this_thread::sleep_for(std::chrono::milliseconds(g_config.GcInterval));
    }
}
