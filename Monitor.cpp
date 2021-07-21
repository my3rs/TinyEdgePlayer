#include "Monitor.h"

#include <fstream>

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

template<typename T>
static T max(std::vector<T> v)
{
    T result = v[0];

    for (const auto& item : v)
    {
        if (item > result)
            result = item;
    }

    return result;
}

void Monitor::ThreadFunc()
{
    // 放在 while 外面减少内存分配
    std::vector<double> cpu;
    std::vector<double> ram;
    std::vector<double> wait_time;
    std::vector<double> other;

    int server_count = 0;   // 上面三个 vector 的 size 应该和这个 server_count 严格一致


    while (!shutdown_)
    {
        double var_cpu = 0;
        double var_ram = 0;
        double var_wait_time = 0;
        double var_other = 0;

        for (const auto& server : servers_)
        {
            double cpu_load = server->GetCpuLoad();
            cpu.push_back(cpu_load);
            ram.push_back(server->GetRamLoad());
            wait_time.push_back(0);
            other.push_back(0);

            //server->PrintStatus();

            server_count++;

            /*
            * 试验调整QPS以使数据图更好看
            */
            if (cpu_load >= 10)
            {
                server->ReduceQps();
            }
            else if (cpu_load <= 1)
            {
                server->RaiseQps();
            }

        }

        /* 求和 */
        double sum_cpu = std::accumulate(cpu.begin(), cpu.end(), 0.0);
        double sum_ram = std::accumulate(ram.begin(), ram.end(), 0.0);
        double sum_wait_time = std::accumulate(wait_time.begin(), wait_time.end(), 0.0);
        double sum_other = std::accumulate(other.begin(), other.end(), 0.0);

        /* 求平均值 */
        double mean_cpu = sum_cpu / server_count;
        double mean_ram = sum_ram / server_count;
        double mean_wait_time = sum_wait_time / server_count;
        double mean_other = sum_other / server_count;

        avg_experiment_data_.emplace_back(std::vector<double>{mean_cpu, mean_ram, mean_wait_time, mean_other});

        /* 求方差 */
        for (int i = 0; i < server_count; ++i)
        {
            var_cpu += std::pow(cpu[i] - mean_cpu, 2);
            var_ram += std::pow(ram[i] - mean_ram, 2);
            var_wait_time += std::pow(wait_time[i] - mean_wait_time, 2);
            var_other += std::pow(other[i] - mean_other, 2);
        }

        var_cpu /= server_count;
        var_ram /= server_count;
        var_wait_time /= server_count;
        var_other /= server_count;

        var_experiment_data_.emplace_back(std::vector<double>{var_cpu, var_ram, var_wait_time, var_other});

        /* 求最大值 */
        max_experiment_data_.emplace_back(std::vector<double>{max(cpu), max(ram), max(wait_time), max(other)});


        cpu.clear();
        ram.clear();
        wait_time.clear();
        other.clear();
        server_count = 0;

        if (shutdown_)       // 如果执行PrintStatus()操作后，shutdown变化，可以立即跳出，无需等待sleep
            return;




        


        std::this_thread::sleep_for(std::chrono::seconds(1));

    }


}

void Monitor::SaveExperimentDataToFile()
{
    std::ofstream avg_file;
    std::ofstream var_file;
    std::ofstream max_file;

    /* 1. avg data */
    avg_file.open(Config::data_file_path + balancer_ + ".avg.txt", std::ios::out | std::ios::trunc);
    avg_file << "平均CPU" << "\t" << "平均RAM" << "\t" << "平均等待时间" << "\t" << "平均other" << std::endl;

    for (const auto& vec : avg_experiment_data_)
    {
        avg_file << vec[0] << "\t" << vec[1] << "\t" << vec[2] << "\t" << vec[3] << std::endl;
    }
    avg_file.close();

    /* 2. var data */
    var_file.open(Config::data_file_path + balancer_ + ".var.txt", std::ios::out | std::ios::trunc);
    var_file << "方差CPU" << "\t" << "方差RAM" << "\t" << "方差等待时间" << "\t" << "方差other" << std::endl;

    for (const auto& vec : var_experiment_data_)
    {
        var_file << vec[0] << "\t" << vec[1] << "\t" << vec[2] << "\t" << vec[3] << std::endl;
    }
    var_file.close();

    /* 3. max data */
    max_file.open(Config::data_file_path + balancer_ + ".max.txt", std::ios::out | std::ios::trunc);
    max_file << "最大CPU" << "\t" << "最大RAM" << "\t" << "最大等待时间" << "\t" << "最大other" << std::endl;

    for (const auto& vec : max_experiment_data_)
    {
        max_file << vec[0] << "\t" << vec[1] << "\t" << vec[2] << "\t" << vec[3] << std::endl;
    }
    max_file.close();

}