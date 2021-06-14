#include <iostream>
#include <vector>
#include "threadpool.h"
#include "Storage.h"
#include "easylogging++.h"
#include "Server.h"

INITIALIZE_EASYLOGGINGPP

std::vector<std::unique_ptr<Server>> server_pool;

std::thread monitor;
bool shutdown = false;

void MonitorFunc()
{
    while (!shutdown)
    {
        for (const auto& server : server_pool)
        {
            server->PrintStatus();
        }

        if (shutdown)       // 如果执行PrintStatus()操作后，shutdown变化，可以立即跳出，无需等待sleep
            return;

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void StopMonitor()
{
    shutdown = true;

    if (monitor.joinable())
        monitor.join();
}

int GenerateTaskTime()
{
    int random = rand() % 100;
    return 100 + random;
}

int SelectOnePoolRandom()
{
    static int max = server_pool.size();
    return rand() % max;
}

void InitPools(int count)
{
    server_pool.emplace_back(new Server(1, 512, 1));
    server_pool.emplace_back(new Server(2, 1024, 2));
    server_pool.emplace_back(new Server(3, 1536, 3));
}

void StopServers()
{
    for (const auto& server : server_pool)
    {
        LOG(INFO) << "server[" << server->GetId() << "] is stopping";
        server->Stop();
    }
}

int main(int argc, char* argv[])
{
    srand(time(nullptr));

    InitPools(1);

    monitor = std::thread(MonitorFunc);

    for (int i = 0; i < 10; ++ i)
    {

        int offset = SelectOnePoolRandom();
        LOG(INFO) << "Task #" << i << "--> pool[" << offset << "]";
        server_pool[offset]->Execute(Task(10, 20));

    }


    StopMonitor();
    StopServers();

    return 0;
}
