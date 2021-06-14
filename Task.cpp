#include "Task.h"


int GenerateRandomTime()
{
    auto seed = std::chrono::steady_clock::now().time_since_epoch().count();
    std::default_random_engine e(seed);
    std::uniform_int_distribution u(50, 150);

    return u(e);
}

int GenerateRandomStorage()
{
    auto seed = std::chrono::steady_clock::now().time_since_epoch().count();
    std::default_random_engine e(seed);
    std::uniform_int_distribution u(10, 50);

    return u(e);
}

Task GenerateRandomTask()
{
    return Task(GenerateRandomTime(), GenerateRandomStorage());
}
