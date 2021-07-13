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
	* ����ģʽ
	*/
	static Balancer& Instance();

	/*
	* ��ʼ�� Balancer
	* 1.��������
	*/
	void Init(const std::vector<std::shared_ptr<Server>>& server_pool);

	/* 
	* set��get���ؾ����㷨 
	*/
	void SetLoadBlanceAlgorithm(LoadBalanceAlgorithm a);
	LoadBalanceAlgorithm GetLoadBlanceAlgorithm();

	/*
	* ����lb_algorithm_ѡ��һ��������
	*/
	std::shared_ptr<Server> SelectOneServer();

	/* 
	* ��ӡͳ����Ϣ��������
	* ÿ���������������������
	*/
	void PrintStatistics();


private:
	LoadBalanceAlgorithm				lb_algorithm_;	// ���ؾ����㷨��Ĭ��Ϊ����㷨
	std::vector<ServerPtr>				servers_;		// server pool
	std::unordered_map<ServerPtr, int>	counter_;		// ������������ͳ��ÿ�������������������
	std::vector<ServerPtr>				server_queue_;	// ���������У��洢��ֵΪ��������`servers_`�еĽǱ�
	bool								is_server_queue_ready_;		// ���`server_queue_`�Ƿ���ã�Ĭ��Ӧ����Ϊfalse

private:
	/*
	* ����ģʽ��˽�л����캯��
	*/
	Balancer();

	/*
	* ���ؾ����㷨��Round Robin
	*/
	ServerPtr SelectServerRoundRobin_();

	/*
	* ���ؾ����㷨��Random
	*/
	ServerPtr SelectServerRandom_();

	/*
	* ���ؾ����㷨��Power of k choices
	*/
	ServerPtr SelectServerPower_();

	/*
	* ����ʵ�ֵĸ��ؾ����㷨��Game
	*/
	ServerPtr SelectServerGame_();


	/*
	 * ���·���������server_queue_
	 * ���¹�����`is_server_queue_ready_`��ֵӦ��Ϊfalse
	 * ������ɺ�`is_server_queue_ready_`��ֵӦ��Ϊtrue
	 */
	void UpdateServerQueue_();

	
};

#endif	// TINYEDGEPLAYER_BALANCER_H