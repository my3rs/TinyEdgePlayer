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
	* ����ģʽ
	*/
	static Balancer& Instance();

	void Init(const std::vector<std::shared_ptr<Server>>& server_pool);

	/* 
	* set��get���ؾ����㷨 
	*/
	void SetLoadBlanceAlgorithm(LoadBalanceAlgorithm a);
	LoadBalanceAlgorithm GetLoadBlanceAlgorithm();

	/*
	* ����lb_algorithm_ѡ��һ��������
	*/
	int SelectOneServer();


private:
	LoadBalanceAlgorithm lb_algorithm_;
	std::vector<std::shared_ptr<Server>> servers_;


private:
	Balancer();


	// TODO: power��game�㷨��û��ʵ��

	/*
	* ���ؾ����㷨��Round Robin
	*/
	int SelectServerRoundRobin_();

	/*
	* ���ؾ����㷨��Random
	*/
	int SelectServerRandom_();

	/*
	* ���ؾ����㷨��Power of k choices
	*/
	int SelectServerPower_();

	/*
	* ����ʵ�ֵĸ��ؾ����㷨��Game
	*/
	int SelectServerGame_();
	
};

#endif	// TINYEDGEPLAYER_BALANCER_H