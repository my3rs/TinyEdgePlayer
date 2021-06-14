#ifndef TINYEDGEPLAYER_SERVER_H
#define TINYEDGEPLAYER_SERVER_H

#include <future>
#include <glog/logging.h>

#include "threadpool.h"
#include "Storage.h"
#include "Task.h"
#include "rate_limiter/rate_limiter.h"

class Server
{
public:

    Server(int cpu, int ram, int id);

    void Stop();


    auto Execute(Task t) -> std::future<bool>;

public:
    int GetId() { return id_; }
    double GetCpuLoad() { return cpu_.load(); }
    int GetTaskQueueSize() { return cpu_.GetTaskQueueSize(); }
    double GetCurrentSpeed() { return cpu_.GetCurrentSpeed_(); }

    void PrintStatus();

    std::string GetStatusLogString();
    void GcFunc();

private:
    int         id_;
    ThreadPool  cpu_;
    Storage     storage_;

    bool        shutdown_;
    RateLimiter rate_limiter_;
};


#endif //TINYEDGEPLAYER_SERVER_H
