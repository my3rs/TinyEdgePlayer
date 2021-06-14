#ifndef TINYEDGEPLAYER_TASK_H
#define TINYEDGEPLAYER_TASK_H


/*
 * 任务请求类和生成“符合不同分布特征的任务请求”函数
 */

#include <random>
#include <chrono>

struct Task
{
    int time;       // 计算开销，单位为ms
    int storage;    // 存储开销，单位为MB

    Task(int t, int s) : time(t), storage(s) {}
};

/*
 * 生成随机的任务时间，单位为ms
 */
int GenerateRandomTime();

/*
 * 生成随机的任务存储开销，单位为MB
 */
int GenerateRandomStorage();

/*
 * 生成随机的任务请求
 */
Task GenerateRandomTask();

#endif //TINYEDGEPLAYER_TASK_H
