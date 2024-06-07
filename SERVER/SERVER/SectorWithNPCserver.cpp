#include <iostream>
#include <array>
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <thread>
#include <vector>
#include <mutex>
#include <unordered_set>
#include <concurrent_priority_queue.h>
#include "protocol.h"

#include "include/lua.hpp"


#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")
#pragma comment(lib, "lua54.lib")
using namespace std;

class SESSION;
constexpr int VIEW_RANGE = 5;
constexpr int SECTOR_SIZE = 5;
int  nSectorX = 0;
int  nSectorY = 0;
class Cell {
public:
	//SectorSize
	float left;
	float right;
	float top;
	float bottom;
	float centerX;
	float centerY;

	std::mutex m;
	std::unordered_set<int> p;

	void AddPlayer(int playernum) {
		m.lock();
		p.insert(playernum);
		m.unlock();
	}
	void PopPlayer(int playernum) {
		m.lock();
		p.erase(playernum);
		m.unlock();
	}
};
Cell* pSector;
Cell* pSectorRef;

int GetSectorIndex(const int& x, const int& y) {
	int idX = x / SECTOR_SIZE;
	int idY = y / SECTOR_SIZE;


	return idX + nSectorX * idY;

}
std::unordered_set<int> adjacentSector(int sectorid) {
	std::unordered_set<int> adjectlist;

	
	for (int y = -1; y < 2; ++y) {
		for (int x = -1; x < 2; ++x) {
			int mysector = sectorid;
			int sector = mysector + x + (nSectorX * y);
			if (x ==0 && y == 0) continue;
			//printf("인덱스: %d \n", sector);
			if (sector >= 0 && sector < nSectorY * nSectorX) {
	
				adjectlist.insert(sector);
			}
		}

	}
	//printf("size %l", adjectlist.size());
	//printf("size %l", );

	return adjectlist;

}

std::unordered_set<int> clipinglist(int x, int y, const std::unordered_set<int>& ref) {
	std::list<std::pair<float, int>> likelylist;
	std::unordered_set<int> adjectlist;

	for (int i : ref) {
		float distance = (pSectorRef[i].centerX - x) * (pSectorRef[i].centerX - x) +
			(pSectorRef[i].centerY - y) * (pSectorRef[i].centerY - y);
		likelylist.emplace_back(distance,i );
	}

	likelylist.sort([](const std::pair<float, int>&  a, const std::pair<float, int>&  b) {
		return a.first < b.first;
		});

	auto p = likelylist.begin();
	for (int i = 0; i < 3; ++i) {
		adjectlist.insert((*p).second);
		p++;
	}
	return adjectlist;
}


//타이머
enum EVENT_TYPE { EV_RANDOM_MOVE,EV_THREE_MOVE,EV_BYE };

struct TIMER_EVENT {
	int obj_id;
	chrono::system_clock::time_point wakeup_time;
	EVENT_TYPE event_id;
	int target_id;
	constexpr bool operator < (const TIMER_EVENT& L) const
	{
		return (wakeup_time > L.wakeup_time);
	}
};

concurrency::concurrent_priority_queue<TIMER_EVENT> timer_queue;

enum COMP_TYPE { OP_ACCEPT, OP_RECV, OP_SEND, OP_NPC_MOVE, OP_AI_HELLO,OP_AI_THREE_MOVE ,OP_AI_BYE };
class OVER_EXP {
public:
	WSAOVERLAPPED _over;
	WSABUF _wsabuf;
	char _send_buf[BUF_SIZE];
	COMP_TYPE _comp_type;
	int _ai_target_obj;
	OVER_EXP()
	{
		_wsabuf.len = BUF_SIZE;
		_wsabuf.buf = _send_buf;
		_comp_type = OP_RECV;
		ZeroMemory(&_over, sizeof(_over));
	}
	OVER_EXP(char* packet)
	{
		_wsabuf.len = packet[0];
		_wsabuf.buf = _send_buf;
		ZeroMemory(&_over, sizeof(_over));
		_comp_type = OP_SEND;
		memcpy(_send_buf, packet, packet[0]);
	}
};

enum S_STATE { ST_FREE, ST_ALLOC, ST_INGAME };
class SESSION {
	OVER_EXP _recv_over;

public:
	mutex _s_lock;
	S_STATE _state;
	atomic_bool	_is_active;		// 주위에 플레이어가 있는가?
	int _id;
	SOCKET _socket;
	short	x, y;
	char	_name[NAME_SIZE];
	unordered_set <int> _view_list;
	mutex	_vll;
	int		_prev_remain;
	int		_last_move_time;

	lua_State* _L;
	mutex	_ll;
public:
	SESSION()
	{
		_id = -1;
		_socket = 0;
		x = y = 0;
		_name[0] = 0;
		_state = ST_FREE;
		_prev_remain = 0;
	}

	~SESSION() {}

	void do_recv()
	{
		DWORD recv_flag = 0;
		memset(&_recv_over._over, 0, sizeof(_recv_over._over));
		_recv_over._wsabuf.len = BUF_SIZE - _prev_remain;
		_recv_over._wsabuf.buf = _recv_over._send_buf + _prev_remain;
		WSARecv(_socket, &_recv_over._wsabuf, 1, 0, &recv_flag,
			&_recv_over._over, 0);
	}

	void do_send(void* packet)
	{
		OVER_EXP* sdata = new OVER_EXP{ reinterpret_cast<char*>(packet) };
		WSASend(_socket, &sdata->_wsabuf, 1, 0, 0, &sdata->_over, 0);
	}
	void send_login_info_packet()
	{
		SC_LOGIN_INFO_PACKET p;
		p.id = _id;
		p.size = sizeof(SC_LOGIN_INFO_PACKET);
		p.type = SC_LOGIN_INFO;
		p.x = x;
		p.y = y;
		do_send(&p);
	}
	void send_move_packet(int c_id);
	void send_add_player_packet(int c_id);
	void send_chat_packet(int c_id, const char* mess);
	void send_remove_player_packet(int c_id)
	{
		_vll.lock();
		if (0 == _view_list.count(c_id)) {
			_vll.unlock();
			return;
		}
		_view_list.erase(c_id);
		_vll.unlock();

		SC_REMOVE_OBJECT_PACKET p;
		p.id = c_id;
		p.size = sizeof(p);
		p.type = SC_REMOVE_OBJECT;
		do_send(&p);
	}
};

HANDLE h_iocp;
array<SESSION, MAX_USER + MAX_NPC> clients;


SOCKET g_s_socket, g_c_socket;
OVER_EXP g_a_over;

bool is_pc(int object_id)
{
	return object_id < MAX_USER;
}

bool is_npc(int object_id)
{
	return !is_pc(object_id);
}


bool can_see(int a, int b)
{

	// int dist = sqrtf((clients[a].x - clients[b].x) * (clients[a].x - clients[b].x)
	//	+ (clients[a].y - clients[b].y) * (clients[a].y - clients[b].y));
	int dist_s = (clients[a].x - clients[b].x) * (clients[a].x - clients[b].x)
		+ (clients[a].y - clients[b].y) * (clients[a].y - clients[b].y);

	return VIEW_RANGE * VIEW_RANGE >= dist_s;

	//if (abs(clients[a].x - clients[b].x) > VIEW_RANGE) return false;
	//return abs(clients[a].y - clients[b].y) <= VIEW_RANGE;
}

void SESSION::send_move_packet(int c_id)
{
	SC_MOVE_OBJECT_PACKET p;
	p.id = c_id;
	p.size = sizeof(SC_MOVE_OBJECT_PACKET);
	p.type = SC_MOVE_OBJECT;
	p.x = clients[c_id].x;
	p.y = clients[c_id].y;
	p.move_time = clients[c_id]._last_move_time;
	do_send(&p);
}
void SESSION::send_chat_packet(int p_id, const char* mess)
{
	SC_CHAT_PACKET packet;
	packet.id = p_id;
	packet.size = sizeof(packet);
	packet.type = SC_CHAT;
	strcpy_s(packet.mess, mess);
	do_send(&packet);
}

void SESSION::send_add_player_packet(int c_id)
{
	_vll.lock();
	if (0 != _view_list.count(c_id)) {
		_vll.unlock();
		return;
	}
	_view_list.insert(c_id);
	_vll.unlock();

	SC_ADD_OBJECT_PACKET add_packet;
	add_packet.id = c_id;
	strcpy_s(add_packet.name, clients[c_id]._name);
	add_packet.size = sizeof(add_packet);
	add_packet.type = SC_ADD_OBJECT;
	add_packet.x = clients[c_id].x;
	add_packet.y = clients[c_id].y;
	do_send(&add_packet);
}

int get_new_client_id()
{
	for (int i = 0; i < MAX_USER; ++i) {
		lock_guard <mutex> ll{ clients[i]._s_lock };
		if (clients[i]._state == ST_FREE)
			return i;
	}
	return -1;
}

void WakeUpNPC(int npc_id, int waker)
{
	OVER_EXP* exover = new OVER_EXP;
	exover->_comp_type = OP_AI_HELLO;
	exover->_ai_target_obj = waker;
	PostQueuedCompletionStatus(h_iocp, 1, npc_id, &exover->_over);

	if (clients[npc_id]._is_active) return;
	bool old_state = false;
	if (false == atomic_compare_exchange_strong(&clients[npc_id]._is_active, &old_state, true))
		return;
	TIMER_EVENT ev{ npc_id, chrono::system_clock::now(), EV_RANDOM_MOVE, 0 };
	timer_queue.push(ev);
}

void process_packet(int c_id, char* packet)
{
	switch (packet[1]) {
	case CS_LOGIN: {
		CS_LOGIN_PACKET* p = reinterpret_cast<CS_LOGIN_PACKET*>(packet);
		strcpy_s(clients[c_id]._name, p->name);
		clients[c_id].x = rand() % W_WIDTH;
		clients[c_id].y = rand() % W_HEIGHT;
		int sectornum = GetSectorIndex(clients[c_id].x, clients[c_id].y);
		
		pSector[sectornum].AddPlayer(c_id);

		clients[c_id].send_login_info_packet();
		{
			lock_guard<mutex> ll{ clients[c_id]._s_lock };
			clients[c_id]._state = ST_INGAME;
		}
		for (auto& pl : clients) {
			{
				lock_guard<mutex> ll(pl._s_lock);
				if (ST_INGAME != pl._state) continue;
			}
			if (pl._id == c_id) continue;
			if (false == can_see(c_id, pl._id))
				continue;
			if (is_pc(pl._id)) pl.send_add_player_packet(c_id);
			else WakeUpNPC(pl._id, c_id);
			clients[c_id].send_add_player_packet(pl._id);
		}
		break;
	}
	case CS_MOVE: {
		CS_MOVE_PACKET* p = reinterpret_cast<CS_MOVE_PACKET*>(packet);
		clients[c_id]._last_move_time = p->move_time;
		short x = clients[c_id].x;
		short y = clients[c_id].y;
		switch (p->direction) {
		case 0: if (y > 0) y--; break;
		case 1: if (y < W_HEIGHT - 1) y++; break;
		case 2: if (x > 0) x--; break;
		case 3: if (x < W_WIDTH - 1) x++; break;
		}
		//과거 섹터
		int preindex = GetSectorIndex(clients[c_id].x, clients[c_id].y);
		clients[c_id].x = x;
		clients[c_id].y = y;
		//현재 섹터
		int nowindex = GetSectorIndex(clients[c_id].x, clients[c_id].y);

		if (preindex != nowindex) {
			//과거 섹터에서 빼주기
			pSector[preindex].PopPlayer(c_id);
			//현재 섹터에 적용
			pSector[nowindex].AddPlayer(c_id);

		}
		unordered_set<int> searchSectorID = adjacentSector(nowindex);
		searchSectorID = clipinglist(x,y , searchSectorID);
		searchSectorID.insert(nowindex);
		//뷰리스트 업데이트
		clients[c_id]._vll.lock();
		unordered_set<int> old_vl = clients[c_id]._view_list;
		clients[c_id]._vll.unlock();

		unordered_set<int> new_vl;

		for (const auto& sectorID : searchSectorID) {

			pSector[sectorID].m.lock();
			std::unordered_set<int> sectorPlayerID = pSector[sectorID].p;
			pSector[sectorID].m.unlock();

			for (const auto& pl : sectorPlayerID) {
				auto& cpl = clients[pl];
				if (cpl._state != ST_INGAME) continue;
				if (cpl._id == c_id) continue;
				if (true == can_see(cpl._id, c_id))
					new_vl.insert(cpl._id);
			}
		}



		clients[c_id].send_move_packet(c_id);

		// ADD_PLAYER
		for (auto& pl : new_vl) {
			auto& cpl = clients[pl];
			if (is_pc(pl)) {
				cpl._vll.lock();
				if (clients[pl]._view_list.count(c_id)) {
					cpl._vll.unlock();
					clients[pl].send_move_packet(c_id);
				}
				else {
					cpl._vll.unlock();
					clients[pl].send_add_player_packet(c_id);
				}
			}
			else WakeUpNPC(pl, c_id);

			if (old_vl.count(pl) == 0)
				clients[c_id].send_add_player_packet(pl);
		}
		// REMOVE_PLAYER
		for (auto& cl : old_vl) {
			if (0 == new_vl.count(cl)) {
				clients[c_id].send_remove_player_packet(cl);
				if(is_pc(cl))
					clients[cl].send_remove_player_packet(c_id);

			}
		}
		//뷰 리스트 업데이트
		clients[c_id]._vll.lock();
		 clients[c_id]._view_list = new_vl;
		clients[c_id]._vll.unlock();


	}
	}
}

void disconnect(int c_id)
{
	clients[c_id]._vll.lock();
	unordered_set <int> vl = clients[c_id]._view_list;
	clients[c_id]._vll.unlock();
	for (auto& p_id : vl) {
		if (is_npc(p_id)) continue;
		auto& pl = clients[p_id];
		{
			lock_guard<mutex> ll(pl._s_lock);
			if (ST_INGAME != pl._state) continue;
		}
		if (pl._id == c_id) continue;
		pl.send_remove_player_packet(c_id);
	}
	closesocket(clients[c_id]._socket);

	lock_guard<mutex> ll(clients[c_id]._s_lock);
	clients[c_id]._state = ST_FREE;
}



void do_npc_random_move(int npc_id)
{
	SESSION& npc = clients[npc_id];
	unordered_set<int> old_vl;
	for (auto& obj : clients) {
		if (ST_INGAME != obj._state) continue;
		if (true == is_npc(obj._id)) continue;
		if (true == can_see(npc._id, obj._id))
			old_vl.insert(obj._id);
	}

	int x = npc.x;
	int y = npc.y;
	switch (rand() % 4) {
	case 0: if (x < (W_WIDTH - 1)) x++; break;
	case 1: if (x > 0) x--; break;
	case 2: if (y < (W_HEIGHT - 1)) y++; break;
	case 3:if (y > 0) y--; break;
	}
	//과거 섹터
	int preindex = GetSectorIndex(npc.x, npc.y);
	npc.x = x;
	npc.y = y;

	//현재 섹터
	int nowindex = GetSectorIndex(npc.x, npc.y);

	if (preindex != nowindex) {
		//과거 섹터에서 빼주기
		pSector[preindex].PopPlayer(npc_id);
		//현재 섹터에 적용
		pSector[nowindex].AddPlayer(npc_id);

	}
	unordered_set<int> searchSectorID = adjacentSector(nowindex);
	searchSectorID = clipinglist(x, y, searchSectorID);
	searchSectorID.insert(nowindex);
	//뷰리스트 업데이트
	clients[npc_id]._vll.lock();
	old_vl = clients[npc_id]._view_list;
	clients[npc_id]._vll.unlock();

	unordered_set<int> new_vl;

	for (const auto& sectorID : searchSectorID) {

		pSector[sectorID].m.lock();
		std::unordered_set<int> sectorPlayerID = pSector[sectorID].p;
		pSector[sectorID].m.unlock();

		for (const auto& id : sectorPlayerID) {
			if (clients[id]._state != ST_INGAME) continue;
			if (id == npc_id) continue;
			if (true == can_see(clients[id]._id, npc_id))
				new_vl.insert(clients[id]._id);
		}
	}

	//섹터 적용
	
	//unordered_set<int> new_vl;
	//for (auto& obj : clients) {
	//	if (ST_INGAME != obj._state) continue;
	//	if (true == is_npc(obj._id)) continue;
	//	if (true == can_see(npc._id, obj._id))
	//		new_vl.insert(obj._id);
	//}

	for (auto pl : new_vl) {
		if (0 == old_vl.count(pl)) {
			// 플레이어의 시야에 등장
			clients[pl].send_add_player_packet(npc._id);
		}
		else {
			// 플레이어가 계속 보고 있음.
			clients[pl].send_move_packet(npc._id);
		}
	}
	///vvcxxccxvvdsvdvds
	for (auto pl : old_vl) {
		if (0 == new_vl.count(pl)) {
			clients[pl]._vll.lock();
			if (0 != clients[pl]._view_list.count(npc._id)) {
				clients[pl]._vll.unlock();
				clients[pl].send_remove_player_packet(npc._id);
			}
			else {
				clients[pl]._vll.unlock();
			}
		}
	}

	clients[npc_id]._vll.lock();
	clients[npc_id]._view_list = new_vl;
	clients[npc_id]._vll.unlock();
}

void worker_thread(HANDLE h_iocp)
{
	while (true) {
		DWORD num_bytes;
		ULONG_PTR key;
		WSAOVERLAPPED* over = nullptr;
		BOOL ret = GetQueuedCompletionStatus(h_iocp, &num_bytes, &key, &over, INFINITE);
		OVER_EXP* ex_over = reinterpret_cast<OVER_EXP*>(over);
		if (FALSE == ret) {
			if (ex_over->_comp_type == OP_ACCEPT) cout << "Accept Error";
			else {
				cout << "GQCS Error on client[" << key << "]\n";
				disconnect(static_cast<int>(key));
				if (ex_over->_comp_type == OP_SEND) delete ex_over;
				continue;
			}
		}

		if ((0 == num_bytes) && ((ex_over->_comp_type == OP_RECV) || (ex_over->_comp_type == OP_SEND))) {
			disconnect(static_cast<int>(key));
			if (ex_over->_comp_type == OP_SEND) delete ex_over;
			continue;
		}

		switch (ex_over->_comp_type) {
		case OP_ACCEPT: {
			int client_id = get_new_client_id();
			if (client_id != -1) {
				{
					lock_guard<mutex> ll(clients[client_id]._s_lock);
					clients[client_id]._state = ST_ALLOC;
				}
				clients[client_id].x = 0;
				clients[client_id].y = 0;
				clients[client_id]._id = client_id;
				clients[client_id]._name[0] = 0;
				clients[client_id]._prev_remain = 0;
				clients[client_id]._socket = g_c_socket;
				CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_c_socket),
					h_iocp, client_id, 0);
				clients[client_id].do_recv();
				g_c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			}
			else {
				cout << "Max user exceeded.\n";
			}
			ZeroMemory(&g_a_over._over, sizeof(g_a_over._over));
			int addr_size = sizeof(SOCKADDR_IN);
			AcceptEx(g_s_socket, g_c_socket, g_a_over._send_buf, 0, addr_size + 16, addr_size + 16, 0, &g_a_over._over);
			break;
		}
		case OP_RECV: {
			int remain_data = num_bytes + clients[key]._prev_remain;
			char* p = ex_over->_send_buf;
			while (remain_data > 0) {
				int packet_size = p[0];
				if (packet_size <= remain_data) {
					process_packet(static_cast<int>(key), p);
					p = p + packet_size;
					remain_data = remain_data - packet_size;
				}
				else break;
			}
			clients[key]._prev_remain = remain_data;
			if (remain_data > 0) {
				memcpy(ex_over->_send_buf, p, remain_data);
			}
			clients[key].do_recv();
			break;
		}
		case OP_SEND:
			delete ex_over;
			break;
		case OP_NPC_MOVE: {
			bool keep_alive = false;

			int sectorindex = GetSectorIndex(clients[key].x, clients[key].y);




			unordered_set<int> searchSectorID = adjacentSector(sectorindex);
			searchSectorID = clipinglist(clients[key].x, clients[key].y, searchSectorID);
			searchSectorID.insert(sectorindex);
			
			for (const auto& sectorID : searchSectorID) {

				pSector[sectorID].m.lock();
				std::unordered_set<int> sectorPlayerID = pSector[sectorID].p;
				pSector[sectorID].m.unlock();

				for (auto id : sectorPlayerID) {
					if (clients[id]._state != ST_INGAME) continue;
					if (is_pc(id)) {
						if (can_see(static_cast<int>(key), id)) {
							keep_alive = true;
							break;
						}
					}
				}



			}

			if (true == keep_alive) {
				do_npc_random_move(static_cast<int>(key));
				TIMER_EVENT ev{ key, chrono::system_clock::now() + 1s, EV_RANDOM_MOVE, 0 };
				timer_queue.push(ev);
			}
			else {
				clients[key]._is_active = false;
			}
			delete ex_over;
		}
						break;
		case OP_AI_HELLO: {
			clients[key]._ll.lock();
			auto L = clients[key]._L;
			lua_getglobal(L, "event_player_move");
			lua_pushnumber(L, ex_over->_ai_target_obj);
			lua_pcall(L, 1, 0, 0);
			//lua_pop(L, 1);
			clients[key]._ll.unlock();
			delete ex_over;

			//만약 랜덤 움직임이라면?
		}
		break;
		case OP_AI_THREE_MOVE: {
			do_npc_random_move(static_cast<int>(key));
		}
		break;
		case  OP_AI_BYE: {

			clients[key]._ll.lock();
			auto L = clients[key]._L;
			lua_getglobal(L, "event_Bye_move");
			lua_pushnumber(L, ex_over->_ai_target_obj);
			lua_pcall(L, 1, 0, 0);
			//lua_pop(L, 1);
			clients[key]._ll.unlock();
			delete ex_over;
			

		}	
		break;
		}
	}
}

int API_get_x(lua_State* L)
{
	int user_id =
		(int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	int x = clients[user_id].x;
	lua_pushnumber(L, x);
	return 1;
}

int API_get_y(lua_State* L)
{
	int user_id =
		(int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	int y = clients[user_id].y;
	lua_pushnumber(L, y);
	return 1;
}

int API_SendMessage(lua_State* L)
{
	int my_id = (int)lua_tointeger(L, -3);
	int user_id = (int)lua_tointeger(L, -2);
	char* mess = (char*)lua_tostring(L, -1);

	lua_pop(L, 4);

	clients[user_id].send_chat_packet(my_id, mess);
	return 0;
}

int API_THREE_MOVE(lua_State* L) {
	int my_id = (int)lua_tointeger(L, -2);
	int target = (int)lua_tointeger(L, -1);

	for (int i = 1; i < 4; ++i) {
		TIMER_EVENT ev{ my_id, chrono::system_clock::now()+ std::chrono::seconds(i), EV_THREE_MOVE, target };
		timer_queue.push(ev);
	}

	TIMER_EVENT ev{ my_id, chrono::system_clock::now() + 3s, EV_BYE, target };
	timer_queue.push(ev);
	return 0;
}

void InitializeNPC()
{
	cout << "NPC intialize begin.\n";
	for (int i = MAX_USER; i < MAX_USER + MAX_NPC; ++i) {
		clients[i].x = rand() % W_WIDTH;
		clients[i].y = rand() % W_HEIGHT;
		int sectornum = GetSectorIndex(clients[i].x, clients[i].y);
		pSector[sectornum].AddPlayer(i);
		clients[i]._id = i;
		clients[i]._is_active = false;
		sprintf_s(clients[i]._name, "NPC%d", i);
		clients[i]._state = ST_INGAME;

		auto L = clients[i]._L = luaL_newstate();
		luaL_openlibs(L);
		luaL_loadfile(L, "npc.lua");
		lua_pcall(L, 0, 0, 0);

		lua_getglobal(L, "set_uid");
		lua_pushnumber(L, i);
		lua_pcall(L, 1, 0, 0);
		// lua_pop(L, 1);// eliminate set_uid from stack after call

		lua_register(L, "API_SendMessage", API_SendMessage);
		lua_register(L, "API_get_x", API_get_x);
		lua_register(L, "API_get_y", API_get_y);
		lua_register(L, "API_THREE_MOVE", API_THREE_MOVE);
	}
	cout << "NPC initialize end.\n";
}

void do_timer()
{
	while (true) {
		TIMER_EVENT ev;
		auto current_time = chrono::system_clock::now();
		if (true == timer_queue.try_pop(ev)) {
			if (ev.wakeup_time > current_time) {
				timer_queue.push(ev);		// 최적화 필요
				// timer_queue에 다시 넣지 않고 처리해야 한다.
				this_thread::sleep_for(1ms);  // 실행시간이 아직 안되었으므로 잠시 대기
				continue;
			}
			switch (ev.event_id) {
			case EV_RANDOM_MOVE: {
				OVER_EXP* ov = new OVER_EXP;
				ov->_comp_type = OP_NPC_MOVE;
				PostQueuedCompletionStatus(h_iocp, 1, ev.obj_id, &ov->_over);
				break;
			}
			case EV_THREE_MOVE: {
				OVER_EXP* ov = new OVER_EXP;
				ov->_comp_type = OP_AI_THREE_MOVE;
				PostQueuedCompletionStatus(h_iocp, 1, ev.obj_id, &ov->_over);
				break;

			}
			case EV_BYE: {
				OVER_EXP* ov = new OVER_EXP;
				ov->_ai_target_obj = ev.target_id;
				ov->_comp_type = OP_AI_BYE;

				PostQueuedCompletionStatus(h_iocp, 1, ev.obj_id, &ov->_over);
				break;
			}
			}

			continue;		// 즉시 다음 작업 꺼내기
		}
		this_thread::sleep_for(1ms);   // timer_queue가 비어 있으니 잠시 기다렸다가 다시 시작
	}
}

int main()
{
	//SECTOR Init
	float  nfSectorX = float(W_WIDTH) / float(SECTOR_SIZE);
	float  nfSectorY = float(W_HEIGHT) / float(SECTOR_SIZE);
	  nSectorX = ceil(nfSectorX);
	  nSectorY = ceil(nfSectorY);

	pSector = new Cell[nSectorX * nSectorY];
	pSectorRef = new Cell[nSectorX * nSectorY];
	//  1 2 3 4 5 .. 순의 셀로 채택
	//  6 7 8 9 10..
	// 

	for (int y = 0; y < nSectorY; ++y) {
		for (int x = 0; x < nSectorX; ++x) {

			pSector[y * nSectorX + x].top = (y )* SECTOR_SIZE;
			pSector[y * nSectorX + x].bottom = (y+1) * SECTOR_SIZE;

			pSector[y * nSectorX + x].left = (x) * SECTOR_SIZE;
			pSector[y * nSectorX + x].right = (x + 1 ) * SECTOR_SIZE;

			pSectorRef[y * nSectorX + x].centerY = ((y)*SECTOR_SIZE + (y + 1) * SECTOR_SIZE) / 2;
			pSectorRef[y * nSectorX + x].centerX = ((x)*SECTOR_SIZE + (x + 1) * SECTOR_SIZE)/2;
			if (x + y * nSectorX == 79) {
				break;
			}
		}
	}
	

	InitializeNPC();


	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);
	g_s_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	SOCKADDR_IN server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT_NUM);
	server_addr.sin_addr.S_un.S_addr = INADDR_ANY;
	bind(g_s_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
	listen(g_s_socket, SOMAXCONN);
	SOCKADDR_IN cl_addr;
	int addr_size = sizeof(cl_addr);



	h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_s_socket), h_iocp, 9999, 0);
	g_c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	g_a_over._comp_type = OP_ACCEPT;
	AcceptEx(g_s_socket, g_c_socket, g_a_over._send_buf, 0, addr_size + 16, addr_size + 16, 0, &g_a_over._over);

	vector <thread> worker_threads;
	int num_threads = std::thread::hardware_concurrency();
	for (int i = 0; i < num_threads; ++i)
		worker_threads.emplace_back(worker_thread, h_iocp);
	thread timer_thread{ do_timer };
	timer_thread.join();
	for (auto& th : worker_threads)
		th.join();
	closesocket(g_s_socket);
	WSACleanup();
}
