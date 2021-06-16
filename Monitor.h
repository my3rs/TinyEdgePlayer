#ifndef TINYEDGEPLAYER_MONITOR_H
#define TINYEDGEPLAYER_MONITOR_H

/*
 * 监测器
 * 持有Server池的引用，周期性调用每一个Server的PrintStatus()函数
 */

#include <thread>
#include <vector>

#include "Server.h"

class Monitor
{
public:
    static Monitor& Instance();     // 单例模式

    void Init(const std::vector<std::shared_ptr<Server>>& server_pool);
    void Stop();

private:
    Monitor();

    void ThreadFunc();

    std::thread monitor_thread_;
    std::vector<std::shared_ptr<Server>> servers_;
    bool shutdown_;
};


#endif //TINYEDGEPLAYER_MONITOR_H
