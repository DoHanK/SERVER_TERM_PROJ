#include "stdafx.h"
#include "OVER_EXP.h"
#include "SESSION.h"
#include "CELL.h"
#include "MapInfoLoad.h"


MapInfoLoad maploader;
std::random_device rd;

void process_packet(int c_id, char* packet);
bool can_see(const SESSION& a, const SESSION& b);
int get_new_client_id(std::array<SESSION, MAX_USER>& clients);
void disconnect(int c_id, std::array<SESSION, MAX_USER+MAX_NPC>& clients);
int GetSectorIndex(const int& x, const int& y);
std::string GetJobString(int job);
void WakeUpNPC(int npc_id, int waker);
bool is_pc(int object_id)
{
	return object_id < MAX_USER;
}

bool is_npc(int object_id)
{
	return !is_pc(object_id);
}
struct TIMER_EVENT {
	int obj_id;
	std::chrono::system_clock::time_point wakeup_time;
	EVENT_TYPE event_id;
	int target_id;
	constexpr bool operator < (const TIMER_EVENT& L) const
	{
		return (wakeup_time > L.wakeup_time);
	}
};
concurrency::concurrent_priority_queue<TIMER_EVENT> timer_queue;



std::array<SESSION, MAX_USER + MAX_NPC> clients;

std::queue<std::pair<int, std::pair<int,char* >>> QueryQueue; // index, OP ,id
std::mutex QueryLock;
SQLHSTMT hstmt = 0;
enum QUERRY_TYPE { OP_GETINFO, OP_SAVEINFO ,OP_CRAETE_ID };



HANDLE g_h_iocp;
SOCKET g_s_socket, g_c_socket;
OVER_EXP g_a_over;
 CELL* pSector = nullptr;
 CELL* pSectorRef = nullptr;

int SECTOR_NUM::nSectorX = 0;
int SECTOR_NUM::nSectorY = 0;

void ConnectDataBase() {
	SQLHENV henv;
	SQLHDBC hdbc;

	SQLRETURN retcode;
	SQLSCHAR UserID[NAME_SIZE];
	SQLINTEGER UserVisual, UserMaxHp, UserHp, UserExp, UserAttack, UserLevel, UserPosX, UserPosY;
	SQLLEN cbVISUAL, cbMAXHP = 0, cbHP = 0, cbEXP = 0, cbATTACK = 0, cbLEVEL = 0, cbID = 0, cbPosX = 0, cbPosY = 0;

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
				retcode = SQLConnect(hdbc, (SQLWCHAR*)L"TermProject", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);
				retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
				// Allocate statement handle  
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					while (true) {
						//std::cout << "루프도는중" << std::endl;
						std::pair<int, std::pair<int,char*>> GetOpNId;

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
							std::wstring cmd = L"EXEC GetUserInfo ";
							std::string tempid = GetOpNId.second.second;
							std::wstring id;
							id.assign(tempid.begin(), tempid.end());
							cmd += id;

							retcode = SQLExecDirect(hstmt, (SQLWCHAR*)cmd.c_str(), SQL_NTS);

							if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {

								// Bind columns 1, 2, and 3  
								retcode = SQLBindCol(hstmt, 1, SQL_C_CHAR, &UserID, 20, &cbID);
								retcode = SQLBindCol(hstmt, 2, SQL_C_LONG, &UserVisual, 4, &cbVISUAL);
								retcode = SQLBindCol(hstmt, 3, SQL_C_LONG, &UserMaxHp, 4, &cbMAXHP);
								retcode = SQLBindCol(hstmt, 4, SQL_C_LONG, &UserHp, 4, &cbHP);
								retcode = SQLBindCol(hstmt, 5, SQL_C_LONG, &UserExp, 4, &cbEXP);
								retcode = SQLBindCol(hstmt, 6, SQL_C_LONG, &UserAttack, 4, &cbATTACK);
								retcode = SQLBindCol(hstmt, 7, SQL_C_LONG, &UserLevel, 4, &cbLEVEL);
								retcode = SQLBindCol(hstmt, 8, SQL_C_LONG, &UserPosX, 4, &cbPosX);
								retcode = SQLBindCol(hstmt, 9, SQL_C_LONG, &UserPosY, 4, &cbPosY);

								// Fetch and print each row of data. On an error, display a message and exit.  
								for (int i = 0; ; i++) {
									retcode = SQLFetch(hstmt);
									/*     if (retcode == SQL_ERROR || retcode == SQL_SUCCESS_WITH_INFO)
											 HandleDiagnosticRecord(hstmt, SQL_HANDLE_STMT, retcode);*/
									if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
									{

								
										clients[GetOpNId.first].m_userid = GetOpNId.second.second;				
										clients[GetOpNId.first].m_visual = UserVisual;
										clients[GetOpNId.first].m_max_hp = UserMaxHp;
										clients[GetOpNId.first].m_hp = UserHp;
										clients[GetOpNId.first].m_exp = UserExp;
										clients[GetOpNId.first].m_attack_damge = UserAttack;
										clients[GetOpNId.first].m_level = UserLevel;
										clients[GetOpNId.first].m_x = UserPosX;
										clients[GetOpNId.first].m_y = UserPosY;

										int sectornum = GetSectorIndex(clients[GetOpNId.first].m_x, clients[GetOpNId.first].m_y);

										pSector[sectornum].AddPlayer(GetOpNId.first);

										clients[GetOpNId.first].Send_Login_Info_Packet();
										{
											std::lock_guard<std::mutex> ll{ clients[GetOpNId.first].m_socket_lock };
											clients[GetOpNId.first].m_state = ST_INGAME;
										}

										for (auto& pl : clients) {
											{
												std::lock_guard<std::mutex> ll(pl.m_socket_lock);
												if (ST_INGAME != pl.m_state) continue;
											}
											if (pl.m_id == GetOpNId.first) continue;
											if (false == can_see(clients[GetOpNId.first], clients[pl.m_id]))
												continue;
											if (is_pc(pl.m_id)) pl.Send_Add_Player_Packet(clients[GetOpNId.first],false);
											else WakeUpNPC(pl.m_id, GetOpNId.first);
											clients[GetOpNId.first].Send_Add_Player_Packet(clients[pl.m_id], false);
										}

										delete  GetOpNId.second.second;
										break;

									}
									else if (retcode == SQL_NO_DATA && i == 0) {
										//데이터가 없으면??
										srand(time(0));
										clients[GetOpNId.first].m_userid = GetOpNId.second.second;
										std::cout << "입장" << clients[GetOpNId.first].m_userid << std::endl;
							
										clients[GetOpNId.first].m_visual = rand()%3;
										clients[GetOpNId.first].m_max_hp = 100;
										clients[GetOpNId.first].m_hp = 100;
										clients[GetOpNId.first].m_exp = 0;
										clients[GetOpNId.first].m_attack_damge = 10;
										clients[GetOpNId.first].m_level = 1;
										clients[GetOpNId.first].m_x = rand()% W_WIDTH;
										clients[GetOpNId.first].m_y = rand() % W_HEIGHT;

										int sectornum = GetSectorIndex(clients[GetOpNId.first].m_x, clients[GetOpNId.first].m_y);

										pSector[sectornum].AddPlayer(GetOpNId.first);

										clients[GetOpNId.first].Send_Login_Info_Packet();
										{
											std::lock_guard<std::mutex> ll{ clients[GetOpNId.first].m_socket_lock };
											clients[GetOpNId.first].m_state = ST_INGAME;
										}

										for (auto& pl : clients) {
											{
												std::lock_guard<std::mutex> ll(pl.m_socket_lock);
												if (ST_INGAME != pl.m_state) continue;
											}
											if (pl.m_id == GetOpNId.first) continue;
											if (false == can_see(clients[GetOpNId.first], clients[pl.m_id]))
												continue;
											if (is_pc(pl.m_id)) pl.Send_Add_Player_Packet(clients[GetOpNId.first], false);
											else WakeUpNPC(pl.m_id, GetOpNId.first);
											clients[GetOpNId.first].Send_Add_Player_Packet(clients[pl.m_id], false);
										}
					/*					char* name = new char[NAME_SIZE];
										memcpy(name, clients[GetOpNId.first].m_userid.c_str(), NAME_SIZE);*/
										//QueryLock.lock();
										//QueryQueue.push({ clients[GetOpNId.first].m_id, {  , name} });
										//QueryLock.unlock();

										//std::cout << "퇴장" << clients[GetOpNId.first].m_userid << std::endl;

										break;
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

							std::wstring cmd = L"EXEC SavePlayerInfo ";
							std::string tempid = GetOpNId.second.second;
							std::wstring id;
							id.assign(tempid.begin(), tempid.end());

							cmd += id;
							cmd += L",";
							cmd += std::to_wstring(clients[GetOpNId.first].m_visual);
							cmd += L",";
							cmd += std::to_wstring(clients[GetOpNId.first].m_max_hp);
							cmd += L",";
							cmd += std::to_wstring(clients[GetOpNId.first].m_hp);
							cmd += L",";
							cmd += std::to_wstring(clients[GetOpNId.first].m_exp);
							cmd += L",";
							cmd += std::to_wstring(clients[GetOpNId.first].m_attack_damge);
							cmd += L",";
							cmd += std::to_wstring(clients[GetOpNId.first].m_level);
							cmd += L",";
							cmd += std::to_wstring(clients[GetOpNId.first].m_x);
							cmd += L",";
							cmd += std::to_wstring(clients[GetOpNId.first].m_y);
							retcode = SQLExecDirect(hstmt, (SQLWCHAR*)cmd.c_str(), SQL_NTS);


						}

						else if (GetOpNId.second.first == OP_CRAETE_ID) {





							delete  GetOpNId.second.second;
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


std::string GetJobString(int job) {
	if (WARRRIOR == job) return "WARRIOR";
	if (ROBOT == job)	return "ROBOT";
	if (NINJA == job) return "NINJA";
	if (WOMANZOMBIE == job) return "여자좀비";
	if (MANZOMBIE == job) return "남자좀비";
	if (DOG == job) return "길강쥐";
	if (DINO == job) return "티노";
	if (CAT == job) return "길냥이";

	return "NONE";
}


bool can_see(const SESSION& a, const SESSION& b)
{
	int dist_s = (a.m_x - b.m_x) * (a.m_x - b.m_x)
		+ (a.m_y - b.m_y) * (a.m_y - b.m_y);

	return VIEW_RANGE * VIEW_RANGE >= dist_s;

}

int get_new_client_id(std::array<SESSION, MAX_USER+ MAX_NPC>& clients)
{
	for (int i = 0; i < MAX_USER; ++i) {
		std::lock_guard <std::mutex> ll{ clients[i].m_socket_lock };
		if (clients[i].m_state == ST_FREE)
			return i;
	}
	return -1;
}

void disconnect(int c_id, std::array<SESSION, MAX_USER+ MAX_NPC>& clients) {

	for (auto& pl : clients) {
		{
			std::lock_guard<std::mutex> ll(pl.m_socket_lock);
			if (ST_INGAME != pl.m_state) continue;
		}
		if (pl.m_id == c_id) continue;
		if (false == can_see(clients[pl.m_id], clients[c_id])) continue;
		pl.Send_Remove_Player_Packet(c_id);
	}
	char* name = new char[NAME_SIZE];
	memcpy(name, clients[c_id].m_userid.c_str(), NAME_SIZE);
	QueryLock.lock();
	QueryQueue.push({ c_id, { OP_SAVEINFO, name } });
	QueryLock.unlock();
	closesocket(clients[c_id].m_socket);

	std::lock_guard<std::mutex> ll(clients[c_id].m_socket_lock);
	clients[c_id].m_state = ST_FREE;

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
					process_packet(static_cast<
						int>(key), p);
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
		char* name = new char[NAME_SIZE];
		memcpy(name, p->userid, NAME_SIZE);
		QueryLock.lock();
		QueryQueue.push({ c_id, { OP_GETINFO ,name } });
		QueryLock.unlock();
		break;
		
	}
	case CS_MOVE: {
		CS_MOVE_PACKET* p = reinterpret_cast<CS_MOVE_PACKET*>(packet);
		clients[c_id].m_last_move_time = p->move_time;
		//std::cout << "클라이언트이동시간: " << clients[c_id].m_last_move_time << std::endl;
		short x = clients[c_id].m_x;
		short y = clients[c_id].m_y;
		switch (p->direction) {
		case 0: if (y > 0) y--; break;
		case 1: if (y < W_HEIGHT - 1) y++; break;
		case 2: if (x > 0) {
			clients[c_id].m_dir = -1;
			x--; break;

		}
		case 3: if (x < W_WIDTH - 1) {
			clients[c_id].m_dir = 1;
			x++; break;
		}
		}
		//충돌처리
		if (maploader.layer[1][y * W_WIDTH + x]) {
			break;
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

		clients[c_id].Send_Move_Packet(clients[c_id]);


		// ADD_PLAYER
		for (auto& cl : new_vl) {

			auto& cpl = clients[cl];
			if (is_pc(cl)) {
				cpl.m_view_lock.lock();

				if (0!= clients[cl].m_view_list.count(c_id)) {
					cpl.m_view_lock.unlock();

					clients[cl].Send_Move_Packet(clients[c_id]);
				}
				else {
					cpl.m_view_list.insert(c_id);
					cpl.m_view_lock.unlock();

					clients[cl].Send_Add_Player_Packet(clients[c_id], true);
				}
			}
			else WakeUpNPC(cl, c_id);

			if (old_vl.count(cl) == 0)
				clients[c_id].Send_Add_Player_Packet(clients[cl],false);
		}
		// REMOVE_PLAYER
		for (auto& cl : old_vl) {
			if (0 == new_vl.count(cl)) { 

				clients[c_id].Send_Remove_Player_Packet(cl);

				if (is_pc(cl)) {
					clients[cl].m_view_lock.lock();
					clients[cl].m_view_list.erase(c_id);
					clients[cl].m_view_lock.unlock();
					clients[cl].Send_Remove_Player_Packet(c_id);

				}
			}
		}

		clients[c_id].m_view_lock.lock();
		clients[c_id].m_view_list = new_vl;
		clients[c_id].m_view_lock.unlock();
		break;
	}
	case CS_CHAT: {
		CS_CHAT_PACKET* p = reinterpret_cast<CS_CHAT_PACKET*>(packet);
		
		std::wstring w(p->mess);
	;
		std::wcout << w << std::endl;

		int nowindex = GetSectorIndex(clients[c_id].m_x, clients[c_id].m_y);
		std::unordered_set<int> searchSectorID = adjacentSector(nowindex);
		searchSectorID = clipinglist(clients[c_id].m_x, clients[c_id].m_y, searchSectorID);
		searchSectorID.insert(nowindex);

		std::unordered_set<int> new_vl;

		for (const auto& sectorID : searchSectorID) {

			pSector[sectorID].m.lock();
			std::unordered_set<int> sectorPlayerID = pSector[sectorID].p;
			pSector[sectorID].m.unlock();

			for (const auto& id : sectorPlayerID) {
				if (clients[id].m_state != ST_INGAME) continue;
				if (id == c_id) continue;
				if (true == can_see(clients[id], clients[c_id]))
					clients[id].Send_Chat_Packet(c_id, p->mess);
			}
		}




		break;
	}
	case CS_ATTACK: {

		int nowindex = GetSectorIndex(clients[c_id].m_x, clients[c_id].m_y);
		std::unordered_set<int> searchSectorID = adjacentSector(nowindex);
		searchSectorID = clipinglist(clients[c_id].m_x, clients[c_id].m_y, searchSectorID);
		searchSectorID.insert(nowindex);

		std::unordered_set<int> new_vl;

		for (const auto& sectorID : searchSectorID) {

			pSector[sectorID].m.lock();
			std::unordered_set<int> sectorPlayerID = pSector[sectorID].p;
			pSector[sectorID].m.unlock();

			for (const auto& id : sectorPlayerID) {
				if (clients[id].m_state != ST_INGAME) continue;
				if (id == c_id) continue;
				if (true == can_see(clients[id], clients[c_id]))
					clients[id].Send_Attack_Packet(c_id);
			}
		}

	
		break;
	}
	case CS_TELEPORT:{
		std::uniform_int_distribution<int> uid(0, maploader.validnode.size());
		
		int select = uid(rd);
		int x = select % 2000;
		int y = select / 2000;
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
		clients[c_id].Send_Move_Packet(clients[c_id]);

		// ADD_PLAYER
		for (auto& cl : new_vl) {

			auto& cpl = clients[cl];
			if (is_pc(cl)) {
				cpl.m_view_lock.lock();

				if (0 != clients[cl].m_view_list.count(c_id)) {
					cpl.m_view_lock.unlock();

					clients[cl].Send_Move_Packet(clients[c_id]);
				}
				else {
					cpl.m_view_list.insert(c_id);
					cpl.m_view_lock.unlock();

					clients[cl].Send_Add_Player_Packet(clients[c_id], true);
				}
			}
			else WakeUpNPC(cl, c_id);

			if (old_vl.count(cl) == 0)
				clients[c_id].Send_Add_Player_Packet(clients[cl], false);
		}
		// REMOVE_PLAYER
		for (auto& cl : old_vl) {
			if (0 == new_vl.count(cl)) {

				clients[c_id].Send_Remove_Player_Packet(cl);

				if (is_pc(cl)) {
					clients[cl].m_view_lock.lock();
					clients[cl].m_view_list.erase(c_id);
					clients[cl].m_view_lock.unlock();
					clients[cl].Send_Remove_Player_Packet(c_id);

				}
			}
		}

		clients[c_id].m_view_lock.lock();
		clients[c_id].m_view_list = new_vl;
		clients[c_id].m_view_lock.unlock();

		break;
	}
	}
}


void InitializeNPC()
{
	std::uniform_int_distribution<int> uid(0, maploader.validnode.size());
	
	std::cout << "NPC intialize begin.\n";
	for (int i = MAX_USER; i < MAX_USER + MAX_NPC; ++i) {
		int index = uid(rd);
		int node = maploader.validnode[index];
		clients[i].m_x = node % W_WIDTH;
		clients[i].m_y = node/ W_HEIGHT;
		int sectornum = GetSectorIndex(clients[i].m_x, clients[i].m_y);

		pSector[sectornum].AddPlayer(i);

		clients[i].m_id = i;
		//몬스터 이름 정해주기
		int monkindnum = 3+rand() % 5;

		clients[i].m_userid = "";
		clients[i].m_userid+=GetJobString(monkindnum);
		clients[i].m_visual = monkindnum;
		clients[i].m_hp = 100;
		clients[i].m_max_hp = 100;

		clients[i].m_state = ST_INGAME;

		auto L = clients[i]._L = luaL_newstate();
		luaL_openlibs(L);
		luaL_loadfile(L, "npc.lua");
		lua_pcall(L, 0, 0, 0);

		lua_getglobal(L, "set_uid");
		lua_pushnumber(L, i);
		lua_pcall(L, 1, 0, 0);
		// lua_pop(L, 1);// eliminate set_uid from stack after call

		//lua_register(L, "API_SendMessage", API_SendMessage);
		//lua_register(L, "API_get_x", API_get_x);
		//lua_register(L, "API_get_y", API_get_y);
	}
	std::cout << "NPC initialize end.\n";
}

void WakeUpNPC(int npc_id, int waker)
{
	OVER_EXP* exover = new OVER_EXP;
	exover->_comp_type = OP_AI_HELLO;
	exover->_ai_target_obj = waker;
	PostQueuedCompletionStatus(g_h_iocp, 1, npc_id, &exover->_over);

	if (clients[npc_id]._is_active) return;
	bool old_state = false;
	
	if (false == std::atomic_compare_exchange_strong(&clients[npc_id]._is_active, &old_state, true))
		return;
	TIMER_EVENT ev{ npc_id, std::chrono::system_clock::now(), EV_RANDOM_MOVE, 0 };
	timer_queue.push(ev);
}

void do_timer()
{
	while (true) {
		TIMER_EVENT ev;
		auto current_time = std::chrono::system_clock::now();
		if (true == timer_queue.try_pop(ev)) {
			if (ev.wakeup_time > current_time) {
				timer_queue.push(ev);		// 최적화 필요
				// timer_queue에 다시 넣지 않고 처리해야 한다.
				std::this_thread::sleep_for(std::chrono::milliseconds(1));  // 실행시간이 아직 안되었으므로 잠시 대기
				continue;
			}
			switch (ev.event_id) {
			case EV_RANDOM_MOVE:
				OVER_EXP* ov = new OVER_EXP;
				ov->_comp_type = OP_NPC_MOVE;
				PostQueuedCompletionStatus(g_h_iocp, 1, ev.obj_id, &ov->_over);
				break;
			}
			continue;		// 즉시 다음 작업 꺼내기
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));    // timer_queue가 비어 있으니 잠시 기다렸다가 다시 시작
	}
}


int main() {

	//SECTOR Init
	float  nfSectorX = float(W_WIDTH) / float(SECTOR_SIZE);
	float  nfSectorY = float(W_HEIGHT) / float(SECTOR_SIZE);
	SECTOR_NUM::nSectorX = ceil(nfSectorX);
	SECTOR_NUM::nSectorY = ceil(nfSectorY);
	
	pSector = new CELL[SECTOR_NUM::nSectorX * SECTOR_NUM::nSectorY];
	pSectorRef = new CELL[SECTOR_NUM::nSectorX * SECTOR_NUM::nSectorY];
	//  1 2 3 4 5 .. 순의 셀로 채택
	//  6 7 8 9 10..
	// 

	for (int y = 0; y < SECTOR_NUM::nSectorY; ++y) {
		for (int x = 0; x < SECTOR_NUM::nSectorX; ++x) {

			pSector[y * SECTOR_NUM::nSectorX + x].top = (y)*SECTOR_SIZE;
			pSector[y * SECTOR_NUM::nSectorX + x].bottom = (y + 1) * SECTOR_SIZE;

			pSector[y * SECTOR_NUM::nSectorX + x].left = (x)*SECTOR_SIZE;
			pSector[y * SECTOR_NUM::nSectorX + x].right = (x + 1) * SECTOR_SIZE;

			pSectorRef[y * SECTOR_NUM::nSectorX + x].centerY = ((y)*SECTOR_SIZE + (y + 1) * SECTOR_SIZE) / 2;
			pSectorRef[y * SECTOR_NUM::nSectorX + x].centerX = ((x)*SECTOR_SIZE + (x + 1) * SECTOR_SIZE) / 2;
			if (x + y * SECTOR_NUM::nSectorX == 79) {
				break;
			}
		}
	}
	std::cout << "맵 로드시작.." << std::endl;
	maploader.Load_Map_info();
	std::cout << "맵 로드 완료 .." << std::endl;

	std::cout << "npc 초기화 시작.." << std::endl;
	InitializeNPC();
	std::cout << "npc 초기화 완료" << std::endl;



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
	g_h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_s_socket), g_h_iocp, 9999, 0);
	g_c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	g_a_over._comp_type = OP_ACCEPT;
	AcceptEx(g_s_socket, g_c_socket, g_a_over._send_buf, 0, addr_size + 16, addr_size + 16, 0, &g_a_over._over);

	std::vector <std::thread> worker_threads;
	int num_threads = std::thread::hardware_concurrency();
	for (int i = 0; i < num_threads; ++i)
		worker_threads.emplace_back(worker_thread, g_h_iocp);

	std::thread DataBase_thread{ ConnectDataBase };
	std::thread timer_thread{ do_timer };

	DataBase_thread.join();
	timer_thread.join();
	for (auto& th : worker_threads)
		th.join();

	closesocket(g_s_socket);
	WSACleanup();
}