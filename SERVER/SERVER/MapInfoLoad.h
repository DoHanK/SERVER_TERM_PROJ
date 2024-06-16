#pragma once
#include <vector>	
#include <fstream>
#include <iostream>
#include <string>

class MapInfoLoad
{public:

	std::vector<bool> layer[2];
	std::vector<int> validnode;
	MapInfoLoad() {


	}

	void Load_Map_info() {

		std::ifstream in{ "MAP.txt" };
		// 파일 열기 실패 여부 확인
		if (!in) {
			std::cerr << "파일을 열지 못했습니다: test1.txt\n";
			if (in.fail()) {
				std::cerr << "오류: 파일을 찾을 수 없습니다 또는 파일을 열 수 없습니다.\n";
			}
			else {
				std::cerr << "알 수 없는 오류가 발생했습니다.\n";
			}

		}

		int layer_index = 0;
		layer[0].reserve(2000 * 2000);
		layer[1].reserve(2000 * 2000);
		char temp;
		std::string sinteger = "";
		int count = 0;
		while (in >> temp) {
			if (temp == ',') {
				int integer = stoi(sinteger);
				if (integer != 0 &&
					integer != 87 && 
					integer != 90 && 
					integer != 93 && 
					integer != 96 && 
					integer != 99 &&
					integer != 301 &&
					integer != 304 &&
					integer != 307 ) {
					layer[layer_index].push_back(true);

				}
				else {
					if(layer_index==1) validnode.push_back(count);

					layer[layer_index].push_back(false);
				}
				sinteger = "";
				count++;
				if (count >=2000 * 2000) {
					layer_index++;
					count = 0;

				}
				continue;
			}
			sinteger += temp;
		}




	}


};
