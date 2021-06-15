#include <iostream>
#include <vector>
#include <glog/logging.h>
#include "threadpool.h"
#include "Storage.h"
#include "Server.h"


std::vector<std::unique_ptr<Server>> server_pool;
std::vector<std::thread> clients;

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

void SendRequest()
{
    int offset = SelectOnePoolRandom();     // 选择处理请求的服务器

    auto task = GenerateRandomTask();       // 生成任务请求


    std::ostringstream client_id;
    client_id << std::this_thread::get_id();


    std::string log;
    log =  "Task # ---> pool[" + std::to_string(offset) + "]";

//    LOG(INFO) << log;
    server_pool[offset]->Execute(task);  // 由上一步选择的服务器处理生成的请求
}

void InitClients()
{
    for (int j = 0; j < 500; ++ j)
    {
        clients.emplace_back(std::thread([](){
            for (int i = 0; i < 10; ++ i)
            {
                SendRequest();
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
        }));
    }
}

void StopClients()
{
    for (auto& t : clients)
    {
        if (t.joinable())
            t.join();
    }
}

int main(int argc, char* argv[])
{
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = true;
    FLAGS_log_prefix = false;

    srand(time(nullptr));

    InitPools(1);

    monitor = std::thread(MonitorFunc);

    InitClients();
    StopClients();


    // 先停止Server，再停止Monitor
    StopServers();

    StopMonitor();


    google::ShutdownGoogleLogging();

    return 0;
}
