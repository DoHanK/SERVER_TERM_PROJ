#include "stdafx.h"
#include "OVER_EXP.h"
#include "SESSION.h"
#include "CELL.h"

void process_packet(int c_id, char* packet);

std::array<SESSION, MAX_USER> clients;

SOCKET g_s_socket, g_c_socket;
OVER_EXP g_a_over;
 CELL* pSector = nullptr;
 CELL* pSectorRef = nullptr;


int SECTOR_NUM::nSectorX = 0;
int SECTOR_NUM::nSectorY = 0;



int GetSectorIndex(const int& x, const int& y) {
	int idX = x / SECTOR_SIZE;
	int idY = y / SECTOR_SIZE;


	return idX + SECTOR_NUM::nSectorX * idY;

}


std::unordered_set<int> adjacentSector(int sectorid) {
	std::unordered_set<int> adjectlist;


	for (int y = -1; y < 2; ++y) {
		for (int x = -1; x < 2; ++x) {
			int mysector = sectorid;
			int sector = mysector + x + (SECTOR_NUM::nSectorX * y);
			if (x == 0 && y == 0) continue;
			//printf("인덱스: %d \n", sector);
			if (sector >= 0 && sector < SECTOR_NUM::nSectorY * SECTOR_NUM::nSectorX) {

				adjectlist.insert(sector);
			}
		}

	}


	return adjectlist;

}

std::unordered_set<int> clipinglist(int x, int y, const std::unordered_set<int>& ref) {
	std::list<std::pair<float, int>> likelylist;
	std::unordered_set<int> adjectlist;

	for (int i : ref) {
		float distance = (pSectorRef[i].centerX - x) * (pSectorRef[i].centerX - x) +
			(pSectorRef[i].centerY - y) * (pSectorRef[i].centerY - y);
		likelylist.emplace_back(distance, i);
	}

	likelylist.sort([](const std::pair<float, int>& a, const std::pair<float, int>& b) {
		return a.first < b.first;
		});

	auto p = likelylist.begin();
	for (int i = 0; i < 3; ++i) {
		adjectlist.insert((*p).second);
		p++;
	}
	return adjectlist;
}


bool can_see(const SESSION& a, const SESSION& b)
{
	int dist_s = (a.m_x - b.m_x) * (a.m_x - b.m_x)
		+ (a.m_y - b.m_y) * (a.m_y - b.m_y);

	return VIEW_RANGE * VIEW_RANGE >= dist_s;

}

int get_new_client_id(std::array<SESSION, MAX_USER>& clients)
{
	for (int i = 0; i < MAX_USER; ++i) {
		std::lock_guard <std::mutex> ll{ clients[i].m_socket_lock };
		if (clients[i].m_state == ST_FREE)
			return i;
	}
	return -1;
}

void disconnect(int c_id, std::array<SESSION, MAX_USER>& clients) {



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
			if (ex_over->_comp_type == OP_ACCEPT) std::cout << "Accept Error";
			else {
				std::cout << "GQCS Error on client[" << key << "]\n";
				disconnect(static_cast<int>(key), clients);
				if (ex_over->_comp_type == OP_SEND) delete ex_over;
				continue;
			}
		}

		if ((0 == num_bytes) && ((ex_over->_comp_type == OP_RECV) || (ex_over->_comp_type == OP_SEND))) {
			disconnect(static_cast<int>(key), clients);
			if (ex_over->_comp_type == OP_SEND) delete ex_over;
			continue;
		}

		switch (ex_over->_comp_type) {
		case OP_ACCEPT: {
			int client_id = get_new_client_id(clients);
			if (client_id != -1) {
				{
					std::lock_guard<std::mutex> ll(clients[client_id].m_socket_lock);
					clients[client_id].m_state = ST_ALLOC;
				}
				clients[client_id].m_x = 0;
				clients[client_id].m_y = 0;
				clients[client_id].m_id = client_id;
				clients[client_id].m_username[0] = 0;
				clients[client_id].m_prev_remain = 0;
				clients[client_id].m_socket = g_c_socket;
				CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_c_socket),
					h_iocp, client_id, 0);
				clients[client_id].Recv();
				g_c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			}
			else {
				std::cout << "Max user exceeded.\n";
			}
			ZeroMemory(&g_a_over._over, sizeof(g_a_over._over));
			int addr_size = sizeof(SOCKADDR_IN);
			AcceptEx(g_s_socket, g_c_socket, g_a_over._send_buf, 0, addr_size + 16, addr_size + 16, 0, &g_a_over._over);
			break;
		}
		case OP_RECV: {
			int remain_data = num_bytes + clients[key].m_prev_remain;
			char* p = ex_over->_send_buf;
			while (remain_data > 0) {
				int packet_size = MAKEWORD(p[0], p[1]);
				if (packet_size <= remain_data) {
					process_packet(static_cast<int>(key), p);
					p = p + packet_size;
					remain_data = remain_data - packet_size;
				}
				else break;
			}
			clients[key].m_prev_remain = remain_data;
			if (remain_data > 0) {
				memcpy(ex_over->_send_buf, p, remain_data);
			}
			clients[key].Recv();
			break;
		}
		case OP_SEND:
			delete ex_over;
			break;
		}
	}
}

void process_packet(int c_id, char* packet)
{
	switch (packet[2]) {
	case CS_LOGIN: {
		CS_LOGIN_PACKET* p = reinterpret_cast<CS_LOGIN_PACKET*>(packet);
		//QueryLock.lock();
		//QueryQueue.push({ c_id, { OP_GETINFO ,p->userid } });
		//QueryLock.unlock();
		break;
	}
	case CS_MOVE: {
		CS_MOVE_PACKET* p = reinterpret_cast<CS_MOVE_PACKET*>(packet);
		clients[c_id].m_last_move_time = p->move_time;
		short x = clients[c_id].m_x;
		short y = clients[c_id].m_y;
		switch (p->direction) {
		case 0: if (y > 0) y--; break;
		case 1: if (y < W_HEIGHT - 1) y++; break;
		case 2: if (x > 0) x--; break;
		case 3: if (x < W_WIDTH - 1) x++; break;
		}
		//과거 섹터
		int preindex = GetSectorIndex(clients[c_id].m_x, clients[c_id].m_y);
		clients[c_id].m_x = x;
		clients[c_id].m_y = y;
		//현재 섹터
		int nowindex = GetSectorIndex(clients[c_id].m_x, clients[c_id].m_y);

		if (preindex != nowindex) {
			//과거 섹터에서 빼주기
			pSector[preindex].PopPlayer(c_id);
			//현재 섹터에 적용
			pSector[nowindex].AddPlayer(c_id);

		}
		std::unordered_set<int> searchSectorID = adjacentSector(nowindex);
		searchSectorID = clipinglist(x, y, searchSectorID);
		searchSectorID.insert(nowindex);
		//뷰리스트 업데이트
		clients[c_id].m_view_lock.lock();
		std::unordered_set<int> old_vl = clients[c_id].m_view_list;
		clients[c_id].m_view_lock.unlock();

		std::unordered_set<int> new_vl;

		for (const auto& sectorID : searchSectorID) {

			pSector[sectorID].m.lock();
			std::unordered_set<int> sectorPlayerID = pSector[sectorID].p;
			pSector[sectorID].m.unlock();

			for (const auto& id : sectorPlayerID) {
				if (clients[id].m_state != ST_INGAME) continue;
				if (id == c_id) continue;
				if (true == can_see(clients[id], clients[c_id]))
					new_vl.insert(clients[id].m_id);
			}
		}
		//clients[c_id].send_move_packet(c_id);

		//// ADD_PLAYER
		//for (auto& cl : new_vl) {
		//	if (0 == old_vl.count(cl)) {
		//		clients[cl].send_add_player_packet(c_id);
		//		clients[c_id].send_add_player_packet(cl);
		//	}
		//	else {
		//		// MOVE_PLAYER
		//		clients[cl].send_move_packet(c_id);
		//	}
		//}
		//// REMOVE_PLAYER
		//for (auto& cl : old_vl) {
		//	if (0 == new_vl.count(cl)) {
		//		clients[cl].send_remove_player_packet(c_id);
		//		clients[c_id].send_remove_player_packet(cl);
		//	}
		//}

	}
	}
}








int main() {


	HANDLE h_iocp;

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

	std::vector <std::thread> worker_threads;
	int num_threads = std::thread::hardware_concurrency();
	for (int i = 0; i < num_threads; ++i)
		worker_threads.emplace_back(worker_thread, h_iocp);


	for (auto& th : worker_threads)
		th.join();

	closesocket(g_s_socket);
	WSACleanup();
}