#include "Task.h"
#include "config.h"

#include <glog/logging.h>


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
    counter ++;
    return Task(GenerateRandomTime(), GenerateRandomStorage());
}


namespace task
{
    void PrintStatistics()
    {
        std::string log_string = "TaskGenerator: generated " + std::to_string(counter) + " tasks";
        LOG(INFO) << log_string;
    }
}