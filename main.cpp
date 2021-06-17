#include <iostream>
#include <vector>
#include <csignal>
#include <glog/logging.h>
#include <gflags/gflags.h>

#include "threadpool.h"
#include "Storage.h"
#include "Server.h"
#include "Monitor.h"

DEFINE_string(balancer, "random", "负载均衡算法，可选值：random, round, game, power");
DEFINE_int32(server, 5, "服务器数量，默认为5");
DEFINE_int32(client, 5, "客户端数量，默认为5");
DEFINE_int32(request, 100, "每个客户端发送的请求数量，默认为100");

// 服务端和客户端
std::vector<std::shared_ptr<Server>> server_pool;
std::vector<std::thread> clients;
bool shutdown = false;      // 控制客户端退出，多个进程间共享即可。只有主进程会对它进行修改


/*
 * 负载均衡算法：随机
 */
int SelectOneServerRandom()
{
    static auto seed = std::chrono::steady_clock::now().time_since_epoch().count();
    static std::default_random_engine engine(seed);
    static int max = server_pool.size();
    static std::uniform_int_distribution u(0, max - 1);     // 注意-1，生成的随机数区间为[min, max]闭区间

    return u(engine);
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
        return SelectOneServerRandom();
    } else if (FLAGS_balancer == "round") {
        return SelectOneServerRoundRobin();
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

    server_pool[offset]->Execute(task);  // 由上一步选择的服务器处理生成的请求
}

/*
 * 初始化客户端
 */
void InitClients()
{
    for (int j = 0; j < FLAGS_client; ++ j)
    {
        clients.emplace_back(std::thread([](){
            for (int i = 0; i < FLAGS_request; ++ i)
            {
                if (shutdown)
                    return;

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
    shutdown = true;

    for (auto& t : clients)
    {
        if (t.joinable())
            t.join();
    }

    LOG(INFO) << "All clients stopped.";
}

void ExitGracefully(int signum = 0) // 异常退出不应该走这个流程
{
    // 停止客户端
    StopClients();

    // 先停止Server，再停止Monitor
    // 顺序不要颠倒，否则在等待Server停止的过程中没有日志输出
    StopServers();
    Monitor::Instance().Stop();

    // 停止glog
    google::ShutdownGoogleLogging();

    exit(signum);
}

void AbnormalSignalHandler(int signum)
{
    LOG(ERROR) << "Interrupt signal (" << signum << ") received. Exiting now...";

    ExitGracefully(signum);
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

    signal(SIGINT, AbnormalSignalHandler);
    signal(SIGABRT, AbnormalSignalHandler);

    // 初始化服务端
    InitPools(1);

    // 初始化监测器
    Monitor::Instance().Init(server_pool);

    // 初始化客户端
    InitClients();


    ExitGracefully(0);
}
