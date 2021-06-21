#include "balancer.h"

#include <random>
#include <chrono>
#include <string>

Balancer::Balancer()
	: lb_algorithm_(LoadBalanceAlgorithm::Random)
{}

Balancer& Balancer::Instance()
{
	static Balancer b;
	return b;
}

void Balancer::Init(const std::vector<std::shared_ptr<Server>>& server_pool)
{
	servers_ = server_pool;

	// 初始化计数器
	for (int i = 0; i < servers_.size(); ++i)
	{
		counter_[i] = 0;
	}
}

void Balancer::SetLoadBlanceAlgorithm(LoadBalanceAlgorithm a)
{
	lb_algorithm_ = a;
}

LoadBalanceAlgorithm Balancer::GetLoadBlanceAlgorithm()
{
	return lb_algorithm_;
}


int Balancer::SelectServerRoundRobin_()
{
	static int offset;
	if (offset >= servers_.size())
		offset = 0;

	return offset++;
}

int Balancer::SelectServerRandom_()
{
	static auto seed = std::chrono::steady_clock::now().time_since_epoch().count();
	static std::default_random_engine engine(seed);
	static int max = servers_.size();
	static std::uniform_int_distribution u(0, max - 1);     // 注意-1，生成的随机数区间为[min, max]闭区间

	return u(engine);
}

int Balancer::SelectServerPower_()
{
	// todo: 实现power算法
	return 0;
}

int Balancer::SelectServerGame_()
{
	// todo: 实现game算法
	return 0;
}

int Balancer::SelectOneServer()
{
	int offset = 0;

	switch (lb_algorithm_)
	{
	case LoadBalanceAlgorithm::Game:
		offset = SelectServerGame_();
		break;

	case LoadBalanceAlgorithm::Power:
		offset = SelectServerPower_();
		break;

	case LoadBalanceAlgorithm::RoundRobin:
		offset = SelectServerRoundRobin_();
		break;

	default:	// 默认使用随机算法
		offset = SelectServerRandom_();
		break;
	}

	counter_[offset] ++;

	return offset;
}

void Balancer::PrintStatistics()
{
	int sum_task = 0;
	std::string log_string = "=================================Statistics======================================\n";

	for (const auto& item : counter_)
	{
		sum_task += item.second;
		log_string += "Server[" + std::to_string(item.first) + "] - " + std::to_string(item.second) + "\n";
	}

	log_string += "SUM - " + std::to_string(sum_task);

	LOG(INFO) << log_string;
}