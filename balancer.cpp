#include "balancer.h"

#include <random>
#include <chrono>

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
	static std::uniform_int_distribution u(0, max - 1);     // ע��-1�����ɵ����������Ϊ[min, max]������

	return u(engine);
}

int Balancer::SelectServerPower_()
{
	// todo: ʵ��power�㷨
	return 0;
}

int Balancer::SelectServerGame_()
{
	// todo: ʵ��game�㷨
	return 0;
}

int Balancer::SelectOneServer()
{
	switch (lb_algorithm_)
	{
	case LoadBalanceAlgorithm::Game:
		return SelectServerGame_();

	case LoadBalanceAlgorithm::Power:
		return SelectServerPower_();

	case LoadBalanceAlgorithm::RoundRobin:
		return SelectServerRoundRobin_();

	default:	// Ĭ��ʹ������㷨
		return SelectServerRandom_();
	}
}