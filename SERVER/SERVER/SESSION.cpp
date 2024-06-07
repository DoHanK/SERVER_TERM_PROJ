#include "SESSION.h"

void SESSION::Recv()
{
	DWORD recv_flag = 0;
	memset(&m_recv_over._over, 0, sizeof(m_recv_over._over));
	m_recv_over._wsabuf.len = BUF_SIZE - m_prev_remain;
	m_recv_over._wsabuf.buf = m_recv_over._send_buf + m_prev_remain;
	WSARecv(m_socket, &m_recv_over._wsabuf, 1, 0, &recv_flag,
		&m_recv_over._over, 0);
}

void SESSION::Send(void* packet)
{
	OVER_EXP* sdata = new OVER_EXP{ reinterpret_cast<char*>(packet) };
	WSASend(m_socket, &sdata->_wsabuf, 1, 0, 0, &sdata->_over, 0);
}

void SESSION::Send_Login_Info_Packet()
{
	SC_LOGIN_INFO_PACKET p;
	p.size = sizeof(SC_LOGIN_INFO_PACKET);
	p.type = SC_LOGIN_INFO;
	p.id = m_id;
	memcpy(p.userid, m_userid.c_str(), NAME_SIZE);
	p.visual = m_visual;
	p.max_hp = m_max_hp;
	p.hp = m_hp;
	p.exp = m_exp;
	p.attack_damge = m_attack_damge;
	p.level = m_level;
	p.x = m_x;
	p.y = m_y;


	
	Send(&p);
}



//Send Packet DataProcessing