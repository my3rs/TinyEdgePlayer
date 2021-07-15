#include "balancer.h"

#include <random>
#include <chrono>
#include <string>
#include <numeric>
#include <algorithm>

Balancer::Balancer()
	: lb_algorithm_(LoadBalanceAlgorithm::Random),
	  is_server_queue_ready_(false)
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
	for (const auto& s : servers_)
	{
		counter_[s] = 0;
	}
}

void Balancer::SetLoadBlanceAlgorithm(LoadBalanceAlgorithm a)
{
	lb_algorithm_ = a;
	
	// 如果使用 game 算法，先生成一个 server queue
	if (a == LoadBalanceAlgorithm::Game)
	{
		UpdateServerQueue_();
	}
}

LoadBalanceAlgorithm Balancer::GetLoadBlanceAlgorithm()
{
	return lb_algorithm_;
}


void Balancer::UpdateServerQueue_()
{
	is_server_queue_ready_ = false;

	server_queue_.clear();

	std::vector<int>	initial_weight;		// 真实权重
	std::vector<int>	current_weight;		// 临时权重

	for (const auto& s : servers_)
	{
		initial_weight.emplace_back(s->GetWeight());
		current_weight.emplace_back(s->GetWeight());
	}

	int sum_weight = std::accumulate(current_weight.begin(), current_weight.end(), 0);

	while (true)
	{
		// 找到临时权重的最大值和拥有该权重的服务器
		auto max_weight_iter = std::max_element(current_weight.begin(), current_weight.end());
		auto max_server_iter = servers_.begin();
		std::advance(max_server_iter, std::distance(current_weight.begin(), max_weight_iter));

		server_queue_.emplace_back(*max_server_iter);

		(*max_weight_iter) -= sum_weight;

		if (server_queue_.size() == sum_weight)
			break;

		for (int i = 0; i < current_weight.size(); ++i)
		{
			current_weight[i] += initial_weight[i];
		}
	}

	is_server_queue_ready_ = true;

	/*if (g_config.Verbose)
	{
		LOG(INFO) << "Updated server queue. queue.size=" << server_queue_.size();
	}*/
}

ServerPtr Balancer::SelectServerRoundRobin_()
{
	static int offset;
	if (offset >= servers_.size())
		offset = 0;

	return servers_[offset++];
}

ServerPtr Balancer::SelectServerRandom_()
{
	static auto seed = std::chrono::steady_clock::now().time_since_epoch().count();
	static std::default_random_engine engine(seed);
	static int max = servers_.size();
	static std::uniform_int_distribution u(0, max - 1);     // 注意-1，生成的随机数区间为[min, max]闭区间

	return servers_[u(engine)];
}

ServerPtr Balancer::SelectServerPower_()
{
	static auto seed = std::chrono::steady_clock::now().time_since_epoch().count();
	static std::default_random_engine engine(seed);
	static int max = servers_.size();
	static std::uniform_int_distribution u(0, max - 1);     // 注意-1，生成的随机数区间为[min, max]闭区间

	int first = u(engine);
	int second = u(engine);

	if (first == second)
		return servers_[first];
	else if (servers_[first]->GetBlockRate() <= servers_[second]->GetBlockRate())
		return servers_[first];
	else
		return servers_[second];
}

ServerPtr Balancer::SelectServerGame_()
{
	static int offset;

	// server queue 用完
	if (offset == server_queue_.size() && offset != 0)
	{
		offset = 0;
		if (is_server_queue_ready_)
		{
			std::thread t([this] {
				UpdateServerQueue_();
				});
			t.detach();
			return SelectServerRoundRobin_();
		}
	}
	else if (!is_server_queue_ready_)
	{
		return SelectServerRoundRobin_();
	}
	else
	{
		return server_queue_[offset++];
	}


	// 如果`server_queue_`没有准备好，需要处理几下情况：
	// 1. 首次运行，`server_queue_`为空 -> 借用轮询算法，同时更新`server_quque_`；
	// 2. 非首次运行，`server_queue_`正在被更新 -> 使用旧的服务队列；
	//if (!is_server_queue_ready_)
	//{
	//	if (g_config.Verbose)
	//		LOG(INFO) << "Server queue is not ready";

	//	if (server_queue_.empty())	// 情况1：首次运行
	//	{
	//		if (g_config.Verbose)
	//			LOG(INFO) << "Updating server queue...";

	//		std::thread t([this]() {
	//			UpdateServerQueue_();
	//			});
	//		t.detach();

	//		return SelectServerRoundRobin_();
	//	}
	//	else						// 情况2：非首次运行
	//	{
	//		if (offset == server_queue_.size())
	//		{
	//			if (g_config.Verbose)
	//				LOG(INFO) << "server queue 用完，重头开始";

	//			offset = 0;
	//			return server_queue_[offset];
	//		}
	//		return server_queue_[offset++];
	//	}
	//}
	//// `server_queue_`准备好的情况
	//else
	//{
	//	if (g_config.Verbose)
	//		LOG(INFO) << "Server queue is ready";

	//	// 到达服务队列末尾
	//	if (offset == server_queue_.size())
	//	{
	//		offset = 0;
	//		return server_queue_[offset];
	//	}
	//	// 没到服务队列末尾，轮询`server_queue_`即可
	//	else
	//	{
	//		return server_queue_[offset++];
	//	}
	//}
}

ServerPtr Balancer::SelectOneServer()
{
	ServerPtr result;

	switch (lb_algorithm_)
	{
	case LoadBalanceAlgorithm::Game:
		result = SelectServerGame_();
		break;

	case LoadBalanceAlgorithm::Power:
		result = SelectServerPower_();
		break;

	case LoadBalanceAlgorithm::RoundRobin:
		result = SelectServerRoundRobin_();
		break;

	default:	// 默认使用随机算法
		result = SelectServerRandom_();
		break;
	}

	counter_[result] ++;

	return result;
}

void Balancer::PrintStatistics()
{
	int sum_task = 0;
	std::string log_string = "=================================Statistics======================================\n";

	for (const auto& item : counter_)
	{
		sum_task += item.second;
		log_string += "Server[" + std::to_string(item.first->GetId()) + "] - " + std::to_string(item.second) + "\n";
	}

	log_string += "SUM - " + std::to_string(sum_task);

	LOG(INFO) << log_string;
}