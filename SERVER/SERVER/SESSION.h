#pragma once
#include "stdafx.h"
#include "OVER_EXP.h"


class SESSION{
public: // 持失切
	SESSION()
	{
		m_id = -1;
		m_socket = 0;
		m_x = m_y = 0;
		m_username[0] = 0;
		m_state = ST_FREE;
		m_prev_remain = 0;
	}
	//社瑚切
	~SESSION() {};

public:
	OVER_EXP				m_recv_over;
public:		//For Data Communication;
	std::mutex				m_socket_lock;
	SOCKET					m_socket;
	S_STATE					m_state;
	int						m_id;
	int						m_prev_remain;



//ForApplication 
	char						m_username[NAME_SIZE];
	std::unordered_set<int>		m_view_list;
	std::mutex					m_view_lock;
	int							m_last_move_time;
public:			//userinfo
	std::string					m_userid;
	int							m_visual;
	short						m_x, m_y;
	int							m_hp;
	int							m_max_hp;
	int							m_attack_damge;
	int							m_exp;
	int							m_level;



public: 
	// func for Data Communication

	void Recv();

	void Send(void* packet);
	

	void Send_Login_Info_Packet();

};

