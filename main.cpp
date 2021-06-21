#include <iostream>
#include <vector>
#include <csignal>

#include <glog/logging.h>
#include <gflags/gflags.h>

#include "threadpool.h"
#include "Storage.h"
#include "Server.h"
#include "Monitor.h"
#include "balancer.h"

// TODO: Client的数量不需太多，当前发送请求的时间时隔还比较大（减少这个间隔以节省线程）

DEFINE_string(balancer, "random", "负载均衡算法，可选值：random, round, game, power");
DEFINE_int32(server, 3, "服务器数量，默认为5");
DEFINE_int32(client, 5, "客户端数量，默认为5");
DEFINE_int32(request, 500, "每个客户端发送的请求数量，默认为100");
DEFINE_bool(verbose, false, "是否打开啰嗦模式");

// 服务端和客户端
std::vector<std::shared_ptr<Server>> server_pool;
std::vector<std::thread> clients;
bool shutdown = false;      // TODO: 控制客户端退出，多个进程间共享即可。只有主进程会对它进行修改



/*
 * 按照命令行参数指定的数量对服务器池进行初始化
 */
void InitPools()
{
    for (int i = 0; i < FLAGS_server; ++ i)
    {
        server_pool.emplace_back(CreateOneServer(i));
    }
}



/*
 * 发送一个请求给客户端
 * 同时具有客户端和负载均衡器的功能
 */
void SendRequest()
{
    auto task = GenerateRandomTask();       // 生成任务请求

    int offset = Balancer::Instance().SelectOneServer();     // 选择处理请求的服务器

    //if (g_config.Verbose)
    //    LOG(INFO) << "Select Server[" << offset << "]";


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
 * 停止所有Server
 */
void StopServers()
{
    for (const auto& server : server_pool)
    {
        LOG(INFO) << "server[" << server->GetId() << "] is stopping";
        server->Stop();
    }
}


/*
 * 在客户端完成任务后，停止所有客户端
 */
void WaitForClientsToEnd()
{

    for (auto& t : clients)
    {
        if (t.joinable())
            t.join();
    }

    LOG(INFO) << "All clients stopped.";
}

/*
 * “优雅”地退出程序，无论收到了什么信号
 */
void ExitGracefully(int signum = 0) // 异常退出不应该走这个流程
{
    if (signum != 0)    // 如果是非正常退出，置shutdown为true以尽快结束clients
        shutdown = true;


    // 等待客户端结束
    WaitForClientsToEnd();

    // 先停止Server，再停止Monitor
    // 顺序不要颠倒，否则在等待Server停止的过程中没有日志输出
    StopServers();
    Monitor::Instance().Stop();

    // 打印统计信息
    Balancer::Instance().PrintStatistics();
    task::PrintStatistics();
    PrintStatus();

    // 停止glog
    google::ShutdownGoogleLogging();

    exit(signum);
}

/*
 * 信号 SIGING, SIGABRT 的 Handler
 */
void AbnormalSignalHandler(int signum)
{
    LOG(ERROR) << "Interrupt signal (" << signum << ") received. Exiting now...";

    ExitGracefully(signum);
}

/*
* 打印状态信息，包括：
* 命令行参数
*/
void PrintStatus()
{
    std::string log_string = "------------命令行参数-------------\n";

    log_string += "服务器数量：" + std::to_string(FLAGS_server) + "\n";
    log_string += "客户端数量：" + std::to_string(FLAGS_client) + "\n";
    log_string += "负载均衡算法：" + FLAGS_balancer + "\n";

    log_string + "-----------------------------------";

    LOG(INFO) << log_string;
}

/*
* 程序入口
*/
int main(int argc, char* argv[])
{
    // 初始化 gflags
    google::ParseCommandLineFlags(&argc, &argv, true);

    // 初始化 glog
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = true;
    FLAGS_log_prefix = false;

    if (FLAGS_verbose)
        g_config.Verbose = true;

    // 注册手动停止程序的信号handler
    signal(SIGINT, AbnormalSignalHandler);
    signal(SIGABRT, AbnormalSignalHandler);

    // 初始化服务端
    InitPools();

    // 初始化负载均衡器
    Balancer::Instance().Init(server_pool);
    if (FLAGS_balancer == "round")
        Balancer::Instance().SetLoadBlanceAlgorithm(LoadBalanceAlgorithm::RoundRobin);
    else if (FLAGS_balancer == "power")
        Balancer::Instance().SetLoadBlanceAlgorithm(LoadBalanceAlgorithm::Power);
    else if (FLAGS_balancer == "game")
        Balancer::Instance().SetLoadBlanceAlgorithm(LoadBalanceAlgorithm::Game);
    else
        Balancer::Instance().SetLoadBlanceAlgorithm(LoadBalanceAlgorithm::Random);


    // 初始化监测器
    Monitor::Instance().Init(server_pool);

    // 初始化客户端
    InitClients();

    ExitGracefully(0);
}
