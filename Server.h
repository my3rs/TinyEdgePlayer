#ifndef TINYEDGEPLAYER_SERVER_H
#define TINYEDGEPLAYER_SERVER_H

#include <future>

#include "threadpool.h"
#include "Storage.h"
#include "Task.h"
#include "easylogging++.h"

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
    void GcFunc();

private:
    int         id_;
    ThreadPool  cpu_;
    Storage     storage_;

    bool        shutdown_;
};


#endif //TINYEDGEPLAYER_SERVER_H
