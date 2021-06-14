#ifndef TINYEDGEPLAYER_TASK_H
#define TINYEDGEPLAYER_TASK_H


struct Task
{
    int time;
    int storage;

    Task(int t, int s) : time(t), storage(s) {}
};


#endif //TINYEDGEPLAYER_TASK_H
