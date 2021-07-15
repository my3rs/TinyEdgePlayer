#ifndef TINYEDGEPLAYER_MONITOR_H
#define TINYEDGEPLAYER_MONITOR_H

/*
 * 监测器
 * 持有Server池的引用，周期性调用每一个Server的PrintStatus()函数
 */

#include <thread>
#include <vector>
#include <string>

#include "Server.h"

class Monitor
{
public:
    static Monitor& Instance();     // 单例模式

    void Init(const std::vector<std::shared_ptr<Server>>& server_pool);
    void Stop();

    void SaveExperimentDataToFile();
    void SetBalancer(std::string b) { balancer_ = b; }

private:
    Monitor();

    void ThreadFunc();

    std::thread monitor_thread_;
    std::vector<std::shared_ptr<Server>> servers_;
    bool shutdown_;

    /* 实验数据 */
    /* 二维数组，每一行都是某个时间点计算的 [CPU, RAM, 等待时间, 占位] */
    std::vector<std::vector<double> > avg_experiment_data_;
    std::vector<std::vector<double> > var_experiment_data_;
    std::vector<std::vector<double> > max_experiment_data_;

    double final_cpu_;
    double final_ram_;
    double final_wait_time_;
    double fineal_other_;

    std::string     balancer_;
};


#endif //TINYEDGEPLAYER_MONITOR_H
