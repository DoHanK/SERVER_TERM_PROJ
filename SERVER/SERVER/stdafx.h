#pragma once
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

// SQLBindCol_ref.cpp  
// compile with: odbc32.lib  
#include <windows.h>  
#include <stdio.h>  
#include <locale.h>






enum S_STATE { ST_FREE, ST_ALLOC, ST_INGAME };
//IOCP ¿¬»ê
enum COMP_TYPE { OP_ACCEPT, OP_RECV, OP_SEND };
constexpr int VIEW_RANGE = 5;
constexpr int SECTOR_SIZE = 5;

class SECTOR_NUM {
public:
	static int nSectorX;
	static int nSectorY;
};



