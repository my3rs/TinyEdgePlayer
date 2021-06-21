#ifndef TINYEDGEPLAYER_BALANCER_H
#define TINYEDGEPLAYER_BALANCER_H

#include <vector>
#include "Server.h"

enum class LoadBalanceAlgorithm
{
	Random,
	RoundRobin,
	Game,
	Power,
};

class Balancer
{
public:
	/*
	* 单例模式
	*/
	static Balancer& Instance();

	void Init(const std::vector<std::shared_ptr<Server>>& server_pool);

	/* 
	* set和get负载均衡算法 
	*/
	void SetLoadBlanceAlgorithm(LoadBalanceAlgorithm a);
	LoadBalanceAlgorithm GetLoadBlanceAlgorithm();

	/*
	* 根据lb_algorithm_选择一个服务器
	*/
	int SelectOneServer();


private:
	LoadBalanceAlgorithm lb_algorithm_;
	std::vector<std::shared_ptr<Server>> servers_;


private:
	Balancer();


	// TODO: power、game算法还没有实现

	/*
	* 负载均衡算法：Round Robin
	*/
	int SelectServerRoundRobin_();

	/*
	* 负载均衡算法：Random
	*/
	int SelectServerRandom_();

	/*
	* 负载均衡算法：Power of k choices
	*/
	int SelectServerPower_();

	/*
	* 本文实现的负载均衡算法：Game
	*/
	int SelectServerGame_();
	
};

#endif	// TINYEDGEPLAYER_BALANCER_H