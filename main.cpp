#include <iostream>
#include <vector>
#include <glog/logging.h>
#include <gflags/gflags.h>

#include "threadpool.h"
#include "Storage.h"
#include "Server.h"

DEFINE_string(balancer, "random", "负载均衡算法，可选值：random, round, game, power");

std::vector<std::unique_ptr<Server>> server_pool;
std::vector<std::thread> clients;

std::thread monitor;
bool shutdown = false;

/*
 * 监测线程的线程函数
 * 负责周期性打印每台服务器的的状态
 */
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

/*
 * 停止监测线程
 */
void StopMonitor()
{
    shutdown = true;

    if (monitor.joinable())
        monitor.join();
}

/*
 * 负载均衡算法：随机
 */
int SelectOnePoolRandom()
{
    static int max = server_pool.size();
    return rand() % max;
}

/*
 * 负载均衡算法：轮询
 */
int SelectOneServerRoundRobin()
{
    static int offset;
    if (offset >= server_pool.size())
        offset = 0;

    return offset ++;
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

/*
 * 根据命令行参数balancer提供的负载均衡算法选择一个服务器
 */
int SelectOneServer()
{
    if (FLAGS_balancer == "random" || FLAGS_balancer == "rand")
    {
        return SelectOnePoolRandom();
    } else if (FLAGS_balancer == "round") {

    }

}

/*
 * 发送一个请求给客户端
 * 同时具有客户端和负载均衡器的功能
 */
void SendRequest()
{
    int offset = SelectOneServer();     // 选择处理请求的服务器

    auto task = GenerateRandomTask();       // 生成任务请求


    std::ostringstream client_id;
    client_id << std::this_thread::get_id();


    std::string log;
    log =  "Task # ---> pool[" + std::to_string(offset) + "]";

//    LOG(INFO) << log;
    server_pool[offset]->Execute(task);  // 由上一步选择的服务器处理生成的请求
}

/*
 * 初始化客户端
 */
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

/*
 * 停止所有客户端
 */
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
    // 初始化 gflags
    google::ParseCommandLineFlags(&argc, &argv, true);

    // 初始化 glog
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = true;
    FLAGS_log_prefix = false;


    srand(time(nullptr));

    // 初始化服务端
    InitPools(1);

    monitor = std::thread(MonitorFunc);

    // 初始化客户端
    InitClients();

    // 停止客户端
    StopClients();


    // 先停止Server，再停止Monitor
    // 顺序不要颠倒，否则在等待Server停止的过程中没有日志输出
    StopServers();
    StopMonitor();


    google::ShutdownGoogleLogging();

    return 0;
}
