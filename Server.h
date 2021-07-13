#ifndef TINYEDGEPLAYER_SERVER_H
#define TINYEDGEPLAYER_SERVER_H

#include <future>
#include <thread>
#include <random>
#include <chrono>
#include <glog/logging.h>

#include "threadpool.h"
#include "Storage.h"
#include "Task.h"
#include "rate_limiter/rate_limiter.h"

class Server
{
public:

    Server(int cpu, int ram, int id);

    /**
     * 停止Server
     */
    void Stop();

    /**
     * 异步执行一个Task
     * @return 包含“Task是否成功执行（是为true，反之false）”的future
     */
    auto Execute(Task t) -> std::future<bool>;

public:
    /**
     * 获得服务器ID
     */
    int     GetId() { return id_; }

    /**
     * 获得CPU使用率
     */
    double  GetCpuLoad() { return cpu_.load(); }

    /*
    * 获得RAM使用率
    */
    double  GetRamLoad() { return storage_.load(); }

    /**
     * 获得近期的平均任务队列长度
     */
    int     GetTaskQueueSize() { return cpu_.GetTaskQueueSize(); }

    /**
     * 获得近期的处理速度
     */
    double  GetCurrentSpeed() { return cpu_.GetCurrentSpeed_(); }

    /**
     * 打印日志
     */
    void    PrintStatus();

    /*
    * set 权重
    */
    void    SetWeight(int w);

    /*
    * get 权重
    */
    int     GetWeight();


private:
    int         id_;        // 服务器序号
    int         weight_;    // 权重，默认值 1
    ThreadPool  cpu_;       // 计算资源
    Storage     storage_;   // 存储资源

    std::thread game_thread_;   // 博弈线程
    bool        shutdown_;      // 用来控制GC线程和博弈的停止。cpu_自己有结束标识，不用这个shutdown_
    RateLimiter rate_limiter_;  // 限流器

private:
    /**
     * 构造日志字符串并返回
     */
    std::string     GetStatusLogString_();

    /**
     * 控制本地资源管理的线程函数
     * 不需要增加单独的线程，由cpu_来执行它即可
     */
    void    GcFunc();
};

/**
 * 返回一个Server的share_ptr指针
 * @id 必需指定服务器ID
 */
std::shared_ptr<Server> CreateOneServer(int id);


#endif //TINYEDGEPLAYER_SERVER_H
