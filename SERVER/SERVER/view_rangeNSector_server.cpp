#include <iostream>
#include <array>
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <thread>
#include <vector>
#include <mutex>
#include <unordered_set>
#include <queue>
#include <string>
#include "protocol.h"
#define UNICODE  
#include <sqlext.h>  


#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")
using namespace std;

void ConnectDataBase();

// SQLBindCol_ref.cpp  
// compile with: odbc32.lib  
#include <windows.h>  
#include <stdio.h>  
#include <locale.h>
class SESSION;
std::queue<pair<int,pair<int, int>>> QueryQueue; // index, OP ,id
mutex QueryLock;
SQLHSTMT hstmt = 0;
enum QUERRY_TYPE { OP_GETINFO, OP_SAVEINFO };

void HandleDiagnosticRecord(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode)
{
	SQLSMALLINT iRec = 0;
	SQLINTEGER iError;
	WCHAR wszMessage[1000];
	WCHAR wszState[SQL_SQLSTATE_SIZE + 1];
	if (RetCode == SQL_INVALID_HANDLE) {
		fwprintf(stderr, L"Invalid handle!\n");
		return;
	}
	while (SQLGetDiagRec(hType, hHandle, ++iRec, wszState, &iError, wszMessage,
		(SQLSMALLINT)(sizeof(wszMessage) / sizeof(WCHAR)), (SQLSMALLINT*)NULL) == SQL_SUCCESS) {
		// Hide data truncated..
		if (wcsncmp(wszState, L"01004", 5)) {
			fwprintf(stderr, L"[%5.5s] %s (%d)\n", wszState, wszMessage, iError);
		}
	}
}

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


enum COMP_TYPE { OP_ACCEPT, OP_RECV, OP_SEND };
class OVER_EXP {
public:
	WSAOVERLAPPED _over;
	WSABUF _wsabuf;
	char _send_buf[BUF_SIZE];
	COMP_TYPE _comp_type;
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
	int _id;
	int _userid;
	SOCKET _socket;
	short	x, y;
	char	_name[NAME_SIZE];
	unordered_set <int> _view_list;
	mutex	_vll;
	int		_prev_remain;
	int		_last_move_time;
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
		p.userid = _userid;
		p.size = sizeof(SC_LOGIN_INFO_PACKET);
		p.type = SC_LOGIN_INFO;
		p.x = x;
		p.y = y;
		do_send(&p);
	}
	void send_login_fail_packet() {
		
			SC_LOGIN_FAIL_PACKET p;

		p.size = sizeof(SC_LOGIN_INFO_PACKET);
		p.type = SC_LOGIN_FAIL;

		do_send(&p);
	}
	void send_move_packet(int c_id);
	void send_add_player_packet(int c_id);
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


array<SESSION, MAX_USER> clients;

SOCKET g_s_socket, g_c_socket;
OVER_EXP g_a_over;

bool can_see(int a, int b)
{
	int dist_s = (clients[a].x - clients[b].x) * (clients[a].x - clients[b].x)
		+ (clients[a].y - clients[b].y) * (clients[a].y - clients[b].y);

	return VIEW_RANGE * VIEW_RANGE >= dist_s;

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
	string username = "P" + to_string(clients[c_id]._userid);
	strcpy_s(add_packet.name, username.c_str());
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

void process_packet(int c_id, char* packet)
{
	switch (packet[2]) {
	case CS_LOGIN: {
		CS_LOGIN_PACKET* p = reinterpret_cast<CS_LOGIN_PACKET*>(packet);
		QueryLock.lock();
		QueryQueue.push({ c_id, { OP_GETINFO ,p->userid } });
		QueryLock.unlock();
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

			for (const auto& id : sectorPlayerID) {
				if (clients[id]._state != ST_INGAME) continue;
				if (id == c_id) continue;
				if (true == can_see(clients[id]._id, c_id))
					new_vl.insert(clients[id]._id);
			}
		}
		clients[c_id].send_move_packet(c_id);

		// ADD_PLAYER
		for (auto& cl : new_vl) {
			if (0 == old_vl.count(cl)) {
				clients[cl].send_add_player_packet(c_id);
				clients[c_id].send_add_player_packet(cl);
			}
			else {
				// MOVE_PLAYER
				clients[cl].send_move_packet(c_id);
			}
		}
		// REMOVE_PLAYER
		for (auto& cl : old_vl) {
			if (0 == new_vl.count(cl)) {
				clients[cl].send_remove_player_packet(c_id);
				clients[c_id].send_remove_player_packet(cl);
			}
		}

	}
	}
}

void disconnect(int c_id)
{
	for (auto& pl : clients) {
		{
			lock_guard<mutex> ll(pl._s_lock);
			if (ST_INGAME != pl._state) continue;
		}
		if (pl._id == c_id) continue;
		if (false == can_see(pl._id, c_id)) continue;
		pl.send_remove_player_packet(c_id);
	}
	QueryLock.lock();
	QueryQueue.push({c_id, { OP_SAVEINFO,clients[c_id]._userid }});
	QueryLock.unlock();

	closesocket(clients[c_id]._socket);

	lock_guard<mutex> ll(clients[c_id]._s_lock);
	clients[c_id]._state = ST_FREE;
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
				int packet_size = MAKEWORD(p[0],p[1]);
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
		}
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

	vector <thread> worker_threads;
	int num_threads = std::thread::hardware_concurrency();
	for (int i = 0; i < num_threads; ++i)
		worker_threads.emplace_back(worker_thread, h_iocp);

	thread DataBase_thread{ ConnectDataBase };

	DataBase_thread.join();
	for (auto& th : worker_threads)
		th.join();

	closesocket(g_s_socket);
	WSACleanup();
}


void ConnectDataBase() {
	SQLHENV henv;
	SQLHDBC hdbc;

	SQLRETURN retcode;
	SQLINTEGER  UserID, UserPosX, UserPosY;
	SQLLEN cbID = 0, cbPosX = 0, cbPosY = 0;

	setlocale(LC_ALL, "korean");
	// std::wcout.imbue(std::locale("korean"));

	 // Allocate environment handle  
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

	// Set the ODBC version environment attribute  
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0);


		// Allocate connection handle  
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

			// Set login timeout to 5 seconds  
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

				// Connect to data source  
				retcode = SQLConnect(hdbc, (SQLWCHAR*)L"GameHomework", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);
				retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
				// Allocate statement handle  
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					while (true) {
						//std::cout << "루프도는중" << std::endl;
						pair<int, pair<int, int>> GetOpNId;
				
						QueryLock.lock();
						if (0 < QueryQueue.size()) {
							GetOpNId = QueryQueue.front();
							QueryQueue.pop();
							QueryLock.unlock();

						}
						else {
							QueryLock.unlock();
							continue;
						}
						if (GetOpNId.second.first == OP_GETINFO) {
							wstring cmd = L"EXEC GetPlayerInfo ";
							cmd += to_wstring(GetOpNId.second.second);

							retcode = SQLExecDirect(hstmt, (SQLWCHAR*)cmd.c_str(), SQL_NTS);

							if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {

								// Bind columns 1, 2, and 3  
								retcode = SQLBindCol(hstmt, 1, SQL_C_LONG, &UserID, 4, &cbID);
								retcode = SQLBindCol(hstmt, 2, SQL_C_LONG, &UserPosX, 4, &cbPosX);
								retcode = SQLBindCol(hstmt, 3, SQL_C_LONG, &UserPosY, 4, &cbPosY);

								// Fetch and print each row of data. On an error, display a message and exit.  
								for (int i = 0; ; i++) {
									retcode = SQLFetch(hstmt);
									/*     if (retcode == SQL_ERROR || retcode == SQL_SUCCESS_WITH_INFO)
											 HandleDiagnosticRecord(hstmt, SQL_HANDLE_STMT, retcode);*/
									if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
									{

										clients[GetOpNId.first].x = UserPosX;
										clients[GetOpNId.first].y = UserPosY;
										clients[GetOpNId.first]._userid = UserID;
										int sectornum = GetSectorIndex(clients[GetOpNId.first].x, clients[GetOpNId.first].y);

										pSector[sectornum].AddPlayer(GetOpNId.first);

										clients[GetOpNId.first].send_login_info_packet();
										{
											lock_guard<mutex> ll{ clients[GetOpNId.first]._s_lock };
											clients[GetOpNId.first]._state = ST_INGAME;
										}
										for (auto& pl : clients) {
											{
												lock_guard<mutex> ll(pl._s_lock);
												if (ST_INGAME != pl._state) continue;
											}
											if (pl._id == GetOpNId.first) continue;
											if (false == can_see(pl._id, GetOpNId.first)) continue;

											pl.send_add_player_packet(GetOpNId.first);
											clients[GetOpNId.first].send_add_player_packet(pl._id);
										}

									}
									else if (retcode == SQL_NO_DATA && i == 0) {
										clients[GetOpNId.first].send_login_fail_packet();
										disconnect(GetOpNId.first);

									}
									else
										break;
								}
							}
							else {
								//실패 패킷

							
							}


							retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
						}


						else if (GetOpNId.second.first == OP_SAVEINFO) {

							wstring cmd = L"EXEC SavePlayerInfo ";
							cmd += to_wstring(GetOpNId.second.second);
							cmd += L",";
							cmd += to_wstring(clients[GetOpNId.first].x);
							cmd += L",";
							cmd += to_wstring(clients[GetOpNId.first].y);
							retcode = SQLExecDirect(hstmt, (SQLWCHAR*)cmd.c_str(), SQL_NTS);


						}


						
						//if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {

						//	// Bind columns 1, 2, and 3  
						//	retcode = SQLBindCol(hstmt, 1, SQL_C_LONG, &UserID, 10, &cbID);
						//	retcode = SQLBindCol(hstmt, 2, SQL_C_LONG, &UserPosX, 10, &cbPosX);
						//	retcode = SQLBindCol(hstmt, 3, SQL_C_LONG, &UserPosY, 10, &cbPosY);

						//	// Fetch and print each row of data. On an error, display a message and exit.  
						//	for (int i = 0; ; i++) {
						//		retcode = SQLFetch(hstmt);
						//		/*     if (retcode == SQL_ERROR || retcode == SQL_SUCCESS_WITH_INFO)
						//				 HandleDiagnosticRecord(hstmt, SQL_HANDLE_STMT, retcode);*/
						//		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
						//		{

						//		}
						//		else
						//			break;
						//	}
						//}
						//else {


						//	//HandleDiagnosticRecord(hstmt, SQL_HANDLE_STMT, retcode);
						//}

					}
					// Process data  
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						SQLCancel(hstmt);
						SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
					}

					SQLDisconnect(hdbc);
				}

				SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
			}
		}
		SQLFreeHandle(SQL_HANDLE_ENV, henv);
	}

}