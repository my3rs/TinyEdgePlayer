#ifndef TINYEDGEPLAYER_BALANCER_H
#define TINYEDGEPLAYER_BALANCER_H

#include <vector>
#include "Server.h"

using ServerPtr = std::shared_ptr<Server>;

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

	/*
	* 初始化 Balancer
	* 1.计数器；
	*/
	void Init(const std::vector<std::shared_ptr<Server>>& server_pool);

	/* 
	* set和get负载均衡算法 
	*/
	void SetLoadBlanceAlgorithm(LoadBalanceAlgorithm a);
	LoadBalanceAlgorithm GetLoadBlanceAlgorithm();

	/*
	* 根据lb_algorithm_选择一个服务器
	*/
	std::shared_ptr<Server> SelectOneServer();

	/* 
	* 打印统计信息，包括：
	* 每个服务器处理的请求数量
	*/
	void PrintStatistics();


private:
	LoadBalanceAlgorithm				lb_algorithm_;	// 负载均衡算法，默认为随机算法
	std::vector<ServerPtr>				servers_;		// server pool
	std::unordered_map<ServerPtr, int>	counter_;		// 计数器，用来统计每个服务器处理多少请求
	std::vector<ServerPtr>				server_queue_;	// 服务器队列，存储的值为服务器在`servers_`中的角标
	bool								is_server_queue_ready_;		// 标记`server_queue_`是否可用，默认应该置为false

private:
	/*
	* 单例模式，私有化构造函数
	*/
	Balancer();

	/*
	* 负载均衡算法：Round Robin
	*/
	ServerPtr SelectServerRoundRobin_();

	/*
	* 负载均衡算法：Random
	*/
	ServerPtr SelectServerRandom_();

	/*
	* 负载均衡算法：Power of k choices
	*/
	ServerPtr SelectServerPower_();

	/*
	* 本文实现的负载均衡算法：Game
	*/
	ServerPtr SelectServerGame_();


	/*
	 * 更新服务器队列server_queue_
	 * 更新过程中`is_server_queue_ready_`的值应该为false
	 * 更新完成后`is_server_queue_ready_`的值应该为true
	 */
	void UpdateServerQueue_();

	
};

#endif	// TINYEDGEPLAYER_BALANCER_H