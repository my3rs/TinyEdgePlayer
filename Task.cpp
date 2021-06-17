#include "Task.h"
#include "config.h"

int GenerateRandomTime()
{
    static auto seed = std::chrono::steady_clock::now().time_since_epoch().count();
    static std::default_random_engine e(seed);
    static std::uniform_int_distribution u(Config::kMinRequestTime, Config::kMaxRequestTime);

    return u(e);
}

int GenerateRandomStorage()
{
    static auto seed = std::chrono::steady_clock::now().time_since_epoch().count();
    static std::default_random_engine e(seed);
    static std::uniform_int_distribution u(Config::kMinRequestStorage, Config::kMaxRequestStorage);

    return u(e);
}

Task GenerateRandomTask()
{
    return Task(GenerateRandomTime(), GenerateRandomStorage());
}
