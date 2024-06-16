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

void SESSION::Send_Move_Packet(const SESSION& MovingPlayer)
{
	SC_MOVE_OBJECT_PACKET p;
	p.size = sizeof(SC_MOVE_OBJECT_PACKET);
	p.type = SC_MOVE_OBJECT;
	p.id = MovingPlayer.m_id;
	p.dir = MovingPlayer.m_dir;
	p.move_time;
	p.x = MovingPlayer.m_x;
	p.y = MovingPlayer.m_y;



	Send(&p);


}

void SESSION::Send_Add_Player_Packet(const SESSION& AddingPlayer,bool bmove)
{
	SC_ADD_OBJECT_PACKET p;
	p.size = sizeof(SC_ADD_OBJECT_PACKET);
	p.type = SC_ADD_OBJECT;
	p.id = AddingPlayer.m_id;
	p.dir = AddingPlayer.m_dir;
	p.max_hp = AddingPlayer.m_max_hp;
	p.hp = AddingPlayer.m_hp;

	if (bmove) {
		p.state = WALK;
	}
	else p.state = IDLE;

	if (AddingPlayer.m_userid.length() < 20) {
		strcpy_s(p.name, AddingPlayer.m_userid.c_str());
		for (int i = 0; i < NAME_SIZE; ++i) {
			if (p.name[i] == '\0');
		}
	}
	p.visual = AddingPlayer.m_visual;
	p.x = AddingPlayer.m_x;
	p.y = AddingPlayer.m_y;

	Send(&p);
}


void SESSION::Send_Remove_Player_Packet(int id) {

	SC_REMOVE_OBJECT_PACKET p;
	p.size = sizeof(SC_REMOVE_OBJECT_PACKET);
	p.type = SC_REMOVE_OBJECT;
	p.id = id;

	Send(&p);
}

void SESSION::Send_Chat_Packet(int id, WCHAR mess[CHAT_SIZE])
{
	SC_CHAT_PACKET p;
	p.size = sizeof(SC_CHAT_PACKET);
	wcscpy_s(p.mess, mess);
	p.type = SC_CHAT;
	p.id = id;

	Send(&p);

}

void SESSION::Send_Attack_Packet(int id)
{

	SC_ATTACK_PACKET p;
	p.size = sizeof(SC_ATTACK_PACKET);
	p.type = SC_ATTACK;
	p.id = id;

	Send(&p);
}
