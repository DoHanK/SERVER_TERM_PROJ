#pragma once
#include "stdafx.h"

class CELL
{
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

