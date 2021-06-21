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

	/* 
	* 打印统计信息，包括：
	* 每个服务器处理的请求数量
	*/
	void PrintStatistics();


private:
	LoadBalanceAlgorithm				lb_algorithm_;	// 负载均衡算法
	std::vector<std::shared_ptr<Server>> servers_;		// server pool
	std::unordered_map<int, int>		counter_;		// 计数器，用来统计每个服务器处理多少请求


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