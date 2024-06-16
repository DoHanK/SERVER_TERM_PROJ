#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <iostream>
#include <unordered_map>
#include <Windows.h>
#include <chrono>
#include <fstream>
#include <set>
#include <thread>
#include <functional>
#include <codecvt>
#include "UImanager.h"
using namespace std;

#include "../../SERVER/SERVER/protocol.h"
sf::TcpSocket s_socket;

std::chrono::duration<float> elapsedTime;
chrono::system_clock::time_point pretime;
chrono::system_clock::time_point nowtime;

#define animationframe 10
/// 애니메이션 
class RESOURCEManager {
public:
	sf::Texture* animation[CHARJOBEND][CHARSTATEEND][animationframe];
	sf::Texture* UIImg[UIEND];

	RESOURCEManager(){}

	void LoadCharImg(){
	
		for (int i = 0; i < CHARJOBEND; ++i) {

			for (int j = 0; j < CHARSTATEEND; ++j) {

				for (int k = 0; k < 10; ++k) {
					string addr = "Resource/";
					addr += GetJobString(i);
					addr += "/";
					addr += GetState(j);
					addr += " (";
					addr += to_string(k+1);
					addr += ").png";

					animation[i][j][k] = new sf::Texture;
			
					animation[i][j][k]->loadFromFile(addr);
				}
			}
		}
	
	}

	void LoadUIImg() {
		
		for (int i = 0; i < UIEND; ++i) {
			string addr = "Resource/UI/";
			addr += getstringUI(i);
			addr += ".png";
			UIImg[i] = new sf::Texture;
			UIImg[i]->loadFromFile(addr);
		}

	}

	string GetState(int state) {
		if (IDLE== state) return "Idle";
		if (WALK== state) return "Walk";
		if (ATTACK== state) return "Attack";
	}

	string GetJobString(int job) {
		if (WARRRIOR== job) return "WARRIOR";
		if (ROBOT== job)	return "ROBOT";
		if (NINJA== job) return "NINJA";
		if (WOMANZOMBIE == job) return "WOMANZOMBIE";
		if (MANZOMBIE == job) return "MANZOMBIE";
		if (DOG == job) return "DOG";
		if (DINO == job) return "DINO";
		if (CAT == job) return "CAT";
		
		return "NONE";
	}

	string getstringUI(int UIKind) {
		if (UIKind == grayhp) return "grayhp";
		if (UIKind == redhp) return "redhp";
		if (UIKind == chat) return "chat";
	}

	std::pair<float, float> getUISize(int UIKind) { // width, height
		if (UIKind == grayhp) return {128,15 };
		if (UIKind == redhp) return { 128,15 };
		if (UIKind == chat) return { 100,20};
	}

	 std::wstring GetJobwString(int job) {
		 if (WARRRIOR == job) return L"WARRIOR";
		 if (ROBOT == job)	return L"ROBOT";
		 if (NINJA == job) return L"NINJA";
		 if (WOMANZOMBIE == job) return L"여자좀비";
		 if (MANZOMBIE == job) return L"남자좀비";
		 if (DOG == job) return L"길강쥐";
		 if (DINO == job) return L"티노";
		 if (CAT == job) return L"길냥이";

		 return L"NONE";
	}
};


constexpr auto SCREEN_WIDTH = 16;
constexpr auto SCREEN_HEIGHT = 16;

constexpr auto TILE_WIDTH = 65;
constexpr auto WINDOW_WIDTH = SCREEN_WIDTH * TILE_WIDTH;   // size of window
constexpr auto WINDOW_HEIGHT = SCREEN_WIDTH * TILE_WIDTH;

int g_left_x;
int g_top_y;
int g_myid;
class OBJECT;

sf::RenderWindow* g_window;
UIManager* UImanger;
sf::Font g_font;
sf::Font g_fhangle;
#define  mapcount 14

class OBJECT {
protected:
	bool m_showing;
	sf::Sprite m_sprite;

	sf::Text m_name;
	sf::Text m_chat;
	sf::Text m_LV;
	chrono::system_clock::time_point m_mess_end_time;
public:
	int id;
	int m_x, m_y;
	char name[NAME_SIZE];
	OBJECT(sf::Texture& t, int x, int y, int x2, int y2) {
		m_showing = false;
		m_sprite.setTexture(t);
		m_sprite.setTextureRect(sf::IntRect(x, y, x2, y2));
		set_name("NONAME");
		m_mess_end_time = chrono::system_clock::now();
	}
	OBJECT(sf::Texture& t, int x, int y, int x2, int y2, float sizex, float sizey) {
		m_showing = false;
		m_sprite.setTexture(t);
		m_sprite.setScale(sizex, sizey);
		m_sprite.setTextureRect(sf::IntRect(x, y, x2, y2));
		set_name("NONAME");
		m_mess_end_time = chrono::system_clock::now();
	}

	OBJECT() {
		m_showing = false;
	}
	void show()
	{
		m_showing = true;
	}
	void hide()
	{
		m_showing = false;
	}

	void a_move(int x, int y) {
		m_sprite.setPosition((float)x, (float)y);

	}

	void a_draw() {
		g_window->draw(m_sprite);
	}

	void move(int x, int y) {
		m_x = x;
		m_y = y;
	}
	virtual void draw() {
		if (false == m_showing) return;
		float rx = (m_x - g_left_x) * 65.0f + 1;
		float ry = (m_y - g_top_y) * 65.0f + 1;
		m_sprite.setPosition(rx, ry);
		g_window->draw(m_sprite);
		auto size = m_name.getGlobalBounds();
		if (m_mess_end_time < chrono::system_clock::now()) {
			m_name.setPosition(rx + 32 - size.width / 2, ry - 10);
			g_window->draw(m_name);
		}
		else {
			m_chat.setPosition(rx + 32 - size.width / 2, ry - 10);
			g_window->draw(m_chat);
		}
	}
	void set_name(const char str[]) {
		m_name.setFont(g_font);
		m_name.setString(str);
		if (id < MAX_USER) m_name.setFillColor(sf::Color(255, 255, 255));
		else m_name.setFillColor(sf::Color(255, 0, 0));
		m_name.setStyle(sf::Text::Bold);
	}
	
	void set_name(const WCHAR str[]) {
		m_name.setFont(g_fhangle);
		m_name.setString(str);
		if (id < MAX_USER) m_name.setFillColor(sf::Color(255, 255, 255));
		else m_name.setFillColor(sf::Color(255, 0, 0));
		m_name.setStyle(sf::Text::Bold);
	}
	void set_level(const char str[]) {
		m_LV.setFont(g_font);
		m_LV.setString(str);
		if (id < MAX_USER) m_LV.setFillColor(sf::Color(255, 255, 255));
		else m_LV.setFillColor(sf::Color(255, 0, 0));
		m_LV.setStyle(sf::Text::Bold);
	}

	void set_chat(const char str[]) {
		m_chat.setFont(g_font);
		m_chat.setString(str);
		m_chat.setFillColor(sf::Color(0, 0, 0));
		m_chat.setStyle(sf::Text::Bold);
		m_mess_end_time = chrono::system_clock::now() + chrono::seconds(3);
	}
	void set_chat(const WCHAR str[]) {
		m_chat.setFont(g_fhangle);
		m_chat.setString(str);
		m_chat.setFillColor(sf::Color(0, 0, 0));
		m_chat.setStyle(sf::Text::Bold);
		m_mess_end_time = chrono::system_clock::now() + chrono::seconds(3);
	}

};

enum DRAWLAYER {
	BG,
	OBSTABLE,
	END,
};
std::vector<short> map[DRAWLAYER::END];

class MAPDrawer {
public:
	int mapnums[mapcount] = { 87, 90, 93, 96,99,230,231,286,301,304,307,311,313,332 };

	sf::Texture* maptexture[mapcount];
	std::vector<short> layer[2];
	OBJECT tiles[mapcount];

	MAPDrawer() {


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
		layer[0].reserve(W_WIDTH * W_HEIGHT);
		layer[1].reserve(W_WIDTH * W_HEIGHT);
		char temp;
		string sinteger = "";
		int count = 0;
		while (in >> temp) {
			if (temp == ',') {
				int integer = stoi(sinteger);
				layer[layer_index].push_back(integer);
				sinteger = "";
				count++;
				if (count >= W_WIDTH * W_HEIGHT) {
					layer_index++;
					count = 0;

				}
				continue;
			}
			sinteger += temp;
		}


		for (int i = 0; i < mapcount; ++i) {
			std::string TexAddr = "Resource/Map1/BG_";
			TexAddr += to_string(mapnums[i]);
			TexAddr += ".png";
			maptexture[i] = new sf::Texture;
			maptexture[i]->loadFromFile(TexAddr);

			tiles[i] = OBJECT{ *maptexture[i], 5, 5, TILE_WIDTH, TILE_WIDTH };
		}

	}

	void Draw() {


		//background
		for (int i = 0; i < SCREEN_WIDTH; ++i)
			for (int j = 0; j < SCREEN_HEIGHT; ++j)
			{
				int tile_x = i + g_left_x;
				int tile_y = j + g_top_y;

				if ((tile_x < 0) || (tile_y < 0)) continue;
				if ((tile_x > W_WIDTH) || (tile_y > W_HEIGHT)) continue;

				int idx = tile_x + tile_y * W_WIDTH;
				if (idx < 0 || idx >W_WIDTH * W_HEIGHT) continue;

				switch (layer[DRAWLAYER::BG][idx]) {
				case 87:
					tiles[0].a_move(TILE_WIDTH * i, TILE_WIDTH * j);
					tiles[0].a_draw();
					break;
				case 90:
					tiles[1].a_move(TILE_WIDTH * i, TILE_WIDTH * j);
					tiles[1].a_draw();
					break;
				case 93:
					tiles[2].a_move(TILE_WIDTH * i, TILE_WIDTH * j);
					tiles[2].a_draw();
					break;
				case 96:
					tiles[3].a_move(TILE_WIDTH * i, TILE_WIDTH * j);
					tiles[3].a_draw();
					break;
				case 99:
					tiles[4].a_move(TILE_WIDTH * i, TILE_WIDTH * j);
					tiles[4].a_draw();
					break;
				case 230:
					tiles[5].a_move(TILE_WIDTH * i, TILE_WIDTH * j);
					tiles[5].a_draw();
					break;
				case 231:
					tiles[6].a_move(TILE_WIDTH * i, TILE_WIDTH * j);
					tiles[6].a_draw();
					break;
				case 286:
					tiles[7].a_move(TILE_WIDTH * i, TILE_WIDTH * j);
					tiles[7].a_draw();
					break;
				case 301:
					tiles[8].a_move(TILE_WIDTH * i, TILE_WIDTH * j);
					tiles[8].a_draw();
					break;
				case 304:
					tiles[9].a_move(TILE_WIDTH * i, TILE_WIDTH * j);
					tiles[9].a_draw();
					break;
				case 307:
					tiles[10].a_move(TILE_WIDTH * i, TILE_WIDTH * j);
					tiles[10].a_draw();
					break;
				case 311:
					tiles[11].a_move(TILE_WIDTH * i, TILE_WIDTH * j);
					tiles[11].a_draw();
					break;
				case 313:
					tiles[12].a_move(TILE_WIDTH * i, TILE_WIDTH * j);
					tiles[12].a_draw();
					break;
				case 332:
					tiles[13].a_move(TILE_WIDTH * i, TILE_WIDTH * j);
					tiles[13].a_draw();
					break;
				}
			}

		//obstable
		for (int i = 0; i < SCREEN_WIDTH; ++i)
			for (int j = 0; j < SCREEN_HEIGHT; ++j)
			{
				int tile_x = i + g_left_x;
				int tile_y = j + g_top_y;

				if ((tile_x < 0) || (tile_y < 0)) continue;
				if ((tile_x > W_WIDTH) || (tile_y > W_HEIGHT)) continue;

				int idx = tile_x + tile_y * W_WIDTH;
				if (idx < 0 || idx >W_WIDTH * W_HEIGHT) continue;
				switch (layer[DRAWLAYER::OBSTABLE][idx]) {
				case 87:
					tiles[0].a_move(TILE_WIDTH * i, TILE_WIDTH * j);
					tiles[0].a_draw();
					break;
				case 90:
					tiles[1].a_move(TILE_WIDTH * i, TILE_WIDTH * j);
					tiles[1].a_draw();
					break;
				case 93:
					tiles[2].a_move(TILE_WIDTH * i, TILE_WIDTH * j);
					tiles[2].a_draw();
					break;
				case 96:
					tiles[3].a_move(TILE_WIDTH * i, TILE_WIDTH * j);
					tiles[3].a_draw();
					break;
				case 99:
					tiles[4].a_move(TILE_WIDTH * i, TILE_WIDTH * j);
					tiles[4].a_draw();
					break;
				case 230:
					tiles[5].a_move(TILE_WIDTH * i, TILE_WIDTH * j);
					tiles[5].a_draw();
					break;
				case 231:
					tiles[6].a_move(TILE_WIDTH * i, TILE_WIDTH * j);
					tiles[6].a_draw();
					break;
				case 286:
					tiles[7].a_move(TILE_WIDTH * i, TILE_WIDTH * j);
					tiles[7].a_draw();
					break;
				case 301:
					tiles[8].a_move(TILE_WIDTH * i, TILE_WIDTH * j);
					tiles[8].a_draw();
					break;
				case 304:
					tiles[9].a_move(TILE_WIDTH * i, TILE_WIDTH * j);
					tiles[9].a_draw();
					break;
				case 307:
					tiles[10].a_move(TILE_WIDTH * i, TILE_WIDTH * j);
					tiles[10].a_draw();
					break;
				case 311:
					tiles[11].a_move(TILE_WIDTH * i, TILE_WIDTH * j);
					tiles[11].a_draw();
					break;
				case 313:
					tiles[12].a_move(TILE_WIDTH * i, TILE_WIDTH * j);
					tiles[12].a_draw();
					break;
				case 332:
					tiles[13].a_move(TILE_WIDTH * i, TILE_WIDTH * j);
					tiles[13].a_draw();
					break;
				}
			}

	}

};

RESOURCEManager ResourceManager;
MAPDrawer MapManger;

class CHARECTOR :public OBJECT {
public:
	int m_state = IDLE;
	int m_KeyFrame=0;
	float m_dir = 1.0f;
	float m_sizex = 0.2f;
	float m_sizey = 0.2f;
	float m_animationTime = 0.05f;
	float m_ElapsedanimationTime = 0.0f;
	sf::Sprite animation[CHARJOBEND][CHARSTATEEND][animationframe];
	sf::Sprite UIsprite[UIEND];

public:
	//ingameInfo
	int m_job = 0;
	int m_max_hp = 0;
	int m_hp = 0;
	int m_exp = 0;
	int m_attack = 0;
	int m_level = 0;
	

	CHARECTOR() {};
	CHARECTOR(int x, int y, int x2, int y2, int job) {
		m_showing = false;
		
		set_name("NONAME");
		m_mess_end_time = chrono::system_clock::now();
		m_job = job;


		for (int i = 0; i < CHARJOBEND; ++i) {

			for (int j = 0; j < CHARSTATEEND; ++j) {

				for (int k = 0; k < 10; ++k) {
					animation[i][j][k].setTexture(*ResourceManager.animation[i][j][k]);
					if(m_job == NINJA)
						animation[i][j][k].setTextureRect(sf::IntRect(0, 0, 64, 64));
					else {
						animation[i][j][k].setTextureRect(sf::IntRect(0, 0, 1024, 1024));
					}
				
					animation[i][j][k].setScale(0.2f, 0.2f);
				}
			}
		}

		for (int i = 0; i < UIEND; ++i) {
			UIsprite[i].setTexture(*ResourceManager.UIImg[i]);
			std::pair<int, int> rectsize = ResourceManager.getUISize(i);
			UIsprite[i].setTextureRect(sf::IntRect(0, 0, rectsize.first, rectsize.second));

		}

	}

	virtual void draw() {
		if (false == m_showing) return;

		m_ElapsedanimationTime += elapsedTime.count();
		if (m_ElapsedanimationTime > m_animationTime) {
			m_KeyFrame++;
			if (m_KeyFrame > animationframe-1) {
				m_KeyFrame = 0;
				m_state = IDLE;
			}
			m_ElapsedanimationTime = 0.0f;


		}
		float rx = (m_x - g_left_x) * 65.0f ;
		float ry = (m_y - g_top_y) * 65.0f ;

		animation[m_job][m_state][m_KeyFrame].setPosition(rx + GetPosOffsetX(m_dir), ry + GetPosOffsetY(m_dir));
		animation[m_job][m_state][m_KeyFrame].setScale(m_dir* GetSizeX(), GetSizeY());

		g_window->draw(animation[m_job][m_state][m_KeyFrame]);


		auto size = m_name.getGlobalBounds();
		m_name.setPosition(rx + 32 - size.width / 2, ry -40 + GetPosOffsetY(m_dir));
		g_window->draw(m_name);

		size = m_LV.getGlobalBounds();
		m_LV.setPosition(rx + 32 - size.width / 2, ry +50);
		g_window->draw(m_LV);
		
			if (m_mess_end_time > chrono::system_clock::now()) {
				size = m_chat.getGlobalBounds();
				UIsprite[chat].setTextureRect(sf::IntRect(0, 0, 512, 512));
				int length = m_chat.getString().getSize();
				if(length < 3) {
					UIsprite[chat].setScale(0.3f, 0.2);
				}
				else if (3<=length &&length <9) {
					UIsprite[chat].setScale(0.6f, 0.2);
				}
				else if (9 <= length && length < 15) {
					UIsprite[chat].setScale(0.9f, 0.2);
				}
				else if (15 <= length && length < 21) {
					UIsprite[chat].setScale(1.2f, 0.2);
				}
				else  {
					UIsprite[chat].setScale(1.5f, 0.2);
				}
				
				UIsprite[chat].setPosition(rx + 10 - size.width / 2, ry - 170);
				m_chat.setPosition(rx + 32 - size.width / 2, ry - 140);
				g_window->draw(UIsprite[chat]);
				g_window->draw(m_chat);
			}
		//DrawUI
		auto uisize = ResourceManager.getUISize(grayhp);
			UIsprite[grayhp].setPosition(rx -30 , ry  + GetPosOffsetY(m_dir));
			g_window->draw(UIsprite[grayhp]);
		//체력 받아오기
			uisize = ResourceManager.getUISize(redhp);
			float width  = uisize.first* ((float)m_hp / (float)m_max_hp);
			UIsprite[redhp].setTextureRect(sf::IntRect(0, 0,width, uisize.second));
			UIsprite[redhp].setPosition(rx - 30, ry + GetPosOffsetY(m_dir));
			g_window->draw(UIsprite[redhp]);
	}


	float GetSizeX() {
		if (WARRRIOR == m_job) return 0.2f;
		if (ROBOT == m_job) return 0.2f;
		if (NINJA == m_job) return 0.25f;
		if (WOMANZOMBIE == m_job) return 0.25f;

		if (MANZOMBIE == m_job) return 0.3f;
		if (DOG == m_job) return 0.25f;
		if (DINO == m_job) return 0.25f;

		if (CAT == m_job) return 0.25f;
	}
	float GetSizeY() {
		if (WARRRIOR == m_job) return 0.2f;
		if (ROBOT == m_job) return 0.25f;
		if (NINJA == m_job) return 0.3f;
		if (WOMANZOMBIE == m_job) return 0.25f;
		if (MANZOMBIE == m_job) return 0.3f;
		if (DOG == m_job) return 0.25f;
		if (DINO == m_job) return 0.25f;
		if (CAT == m_job) return 0.25f;
	}
	float GetPosOffsetX(float dir) {
		if (dir > 0) {
			if (WARRRIOR == m_job) return -20.0f;
			if (ROBOT == m_job) return -20.0f;
			if (NINJA == m_job) return -10.0f;
			if (WOMANZOMBIE == m_job) return -25.0f;
			if (MANZOMBIE == m_job) return -15.0f;
			if (DOG == m_job) return -15.0f;
			if (DINO == m_job) return -15.0f;
			if (CAT == m_job) return -15.0f;

		}
		else {
			if (WARRRIOR == m_job) return 85.0f;
			if (ROBOT == m_job) return 85.0f;
			if (NINJA == m_job) return 75.0f;
			if (WOMANZOMBIE == m_job) return 90.0f;
			if (MANZOMBIE == m_job) return 80.0f;
			if (DOG == m_job) return 95.0f;
			if (DINO == m_job) return 85.0f;
			if (CAT == m_job) return 95.0f;

		}

	}
	float GetPosOffsetY(float dir) {
		if (WARRRIOR == m_job) return -65.0f;
		if (ROBOT == m_job) return -65.0f;
		if (NINJA == m_job) return -65.0f;
		if (WOMANZOMBIE == m_job) return -75.0f;
		if (MANZOMBIE == m_job) return -95.0f;
		if (DOG == m_job) return -65.0f;
		if (DINO == m_job) return -65.0f;
		if (CAT == m_job) return -65.0f;



	}

};

CHARECTOR avatar;
unordered_map <int, CHARECTOR> players;


void client_initialize(){

	if (false == g_font.loadFromFile("cour.ttf")) {
		cout << "Font Loading Error!\n";
		exit(-1);
	}
	if (false == g_fhangle.loadFromFile("hangle.ttf")) {
		cout << "Font Loading Error!\n";
		exit(-1);
	}

	MapManger.Load_Map_info();
	ResourceManager.LoadCharImg();
	ResourceManager.LoadUIImg();
	avatar = CHARECTOR{0, 0, 0, 0, 0 };
	
	avatar.move(4, 4);
}

void client_finish()
{
	players.clear();

}

void ProcessPacket(char* ptr)
{
	static bool first_time = true;
	switch (ptr[2])
	{
	case SC_LOGIN_INFO:
	{
		SC_LOGIN_INFO_PACKET * packet = reinterpret_cast<SC_LOGIN_INFO_PACKET*>(ptr);
		g_myid = packet->id;
		avatar.id = g_myid;
		avatar.m_job = packet->visual;
		avatar.m_max_hp = packet->max_hp;
		avatar.m_hp = packet->hp;
		avatar.m_exp = packet->exp;
		avatar.m_attack = packet->attack_damge;
		avatar.m_level = packet->level;	
		std::string stemplevel = "LV:";
		stemplevel += std::to_string(avatar.m_level);
		avatar.set_level(stemplevel.c_str());
		avatar.move(packet->x, packet->y);


		g_left_x = packet->x - SCREEN_WIDTH / 2;
		g_top_y = packet->y - SCREEN_HEIGHT / 2;
		avatar.show();
	break;
	}
	case SC_ADD_OBJECT:
	{
		SC_ADD_OBJECT_PACKET* my_packet = reinterpret_cast<SC_ADD_OBJECT_PACKET*>(ptr);
		int id = my_packet->id;
		if (id == g_myid) {
			avatar.move(my_packet->x, my_packet->y);
			g_left_x = my_packet->x - SCREEN_WIDTH / 2;
			g_top_y = my_packet->y - SCREEN_HEIGHT / 2;
			avatar.show();
		}
		else if (id < MAX_USER) {
			players[id] = CHARECTOR{  0, 0, 64, 64,0 };
			players[id].id = id;
			players[id].m_job = my_packet->visual;
			players[id].m_dir = my_packet->dir;
			players[id].m_state = my_packet->state;
			players[id].m_hp = my_packet->hp;
			players[id].m_max_hp = my_packet->max_hp;
			players[id].move(my_packet->x, my_packet->y);
			players[id].m_level = my_packet->level;   
			std::string stemplevel = "LV:";
			stemplevel += std::to_string(players[id].m_level);
			players[id].set_level(stemplevel.c_str());
			std::string stempid = "ID:";
			stempid += my_packet->name;
			players[id].set_name(stempid.c_str());
			players[id].show();
		}
		else {
			players[id] = CHARECTOR{ 0, 0, 64, 64,0 };
			players[id].id = id;
			players[id].m_job = my_packet->visual;
			players[id].m_dir = my_packet->dir;
			players[id].m_state = my_packet->state;
			players[id].m_hp = my_packet->hp;
			players[id].m_max_hp = my_packet->max_hp;
			players[id].move(my_packet->x, my_packet->y);

			players[id].m_level = my_packet->level;
			std::string stemplevel = "LV:";
			stemplevel += std::to_string(players[id].m_level);
			players[id].set_level(stemplevel.c_str());
			std::wstring stempid = L"";
			stempid += ResourceManager.GetJobwString(my_packet->visual);
			players[id].set_name(stempid.c_str());
			players[id].show();
		}		
		break;
	}
	case SC_MOVE_OBJECT:
	{
		SC_MOVE_OBJECT_PACKET* my_packet = reinterpret_cast<SC_MOVE_OBJECT_PACKET*>(ptr);
		int other_id = my_packet->id;
		if (other_id == g_myid) {
			avatar.move(my_packet->x, my_packet->y);
			g_left_x = my_packet->x - SCREEN_WIDTH/2;
			g_top_y = my_packet->y - SCREEN_HEIGHT/2;
		}
		else {
			players[other_id].m_state = WALK;
			players[other_id].m_dir = my_packet->dir;
			players[other_id].move(my_packet->x, my_packet->y);
			
		}
		players[other_id].show();
		break;
	}
	case SC_REMOVE_OBJECT:
	{
		SC_REMOVE_OBJECT_PACKET* my_packet = reinterpret_cast<SC_REMOVE_OBJECT_PACKET*>(ptr);
		int other_id = my_packet->id;
		if (other_id == g_myid) {
			avatar.hide();
		}
		else {
			players.erase(other_id);
		}
		break;
	}
	case SC_CHAT:
	{
		SC_CHAT_PACKET* my_packet = reinterpret_cast<SC_CHAT_PACKET*>(ptr);
		int other_id = my_packet->id;
		if (other_id == g_myid) {
			//avatar.set_chat(my_packet->mess);
		}
		else {
			players[other_id].set_chat(my_packet->mess);
		
			
		}

		break;
	}
	case SC_ATTACK: {
		SC_ATTACK_PACKET* my_packet = reinterpret_cast<SC_ATTACK_PACKET*>(ptr);
		int other_id = my_packet->id;
		players[other_id].m_state = ATTACK;
		break;
	}
	case SC_STAT_CHANGE: {
		SC_STAT_CHANGE_PACKET* my_packet = reinterpret_cast<SC_STAT_CHANGE_PACKET*>(ptr);
		int other_id = my_packet->id;
		if (other_id == g_myid) {
			avatar.m_exp = my_packet->exp;
			avatar.m_hp = my_packet->hp;
			avatar.m_level = my_packet->level;
			avatar.m_max_hp = my_packet->max_hp;
			std::string stemplevel = "LV:";
			stemplevel += std::to_string(avatar.m_level);
			avatar.set_level(stemplevel.c_str());
		}
		else {
			players[other_id].m_exp = my_packet->exp;
			players[other_id].m_hp = my_packet->hp;
			players[other_id].m_level = my_packet->level;
			players[other_id].m_max_hp = my_packet->max_hp;

			std::string stemplevel = "LV:";
			stemplevel += std::to_string(players[other_id].m_level);
			players[other_id].set_level(stemplevel.c_str());
		
		}
		break;
	}
	case SC_LOGIN_FAIL: {
		
		exit(0);
	}
	default:
		printf("Unknown PACKET type [%d]\n", ptr[1]);
	}
}

void process_data(char* net_buf, size_t io_byte)
{
	char* ptr = net_buf;
	static size_t in_packet_size = 0;
	static size_t saved_packet_size = 0;
	static char packet_buffer[BUF_SIZE];

	while (0 != io_byte) {
		if (0 == in_packet_size) in_packet_size = MAKEWORD(ptr[0], ptr[1]);
		if (io_byte + saved_packet_size >= in_packet_size) {
			memcpy(packet_buffer + saved_packet_size, ptr, in_packet_size - saved_packet_size);
			ProcessPacket(packet_buffer);
			ptr += in_packet_size - saved_packet_size;
			io_byte -= in_packet_size - saved_packet_size;
			in_packet_size = 0;
			saved_packet_size = 0;
		}
		else {
			memcpy(packet_buffer + saved_packet_size, ptr, io_byte);
			saved_packet_size += io_byte;
			io_byte = 0;
		}
	}
}

void client_main()
{
	char net_buf[BUF_SIZE];
	size_t	received;
	auto recv_result = s_socket.receive(net_buf, BUF_SIZE, received);

	if (recv_result == sf::Socket::Error)
	{
		wcout << L"Recv 에러!";
		exit(-1);
	}
	if (recv_result == sf::Socket::Disconnected) {
		wcout << L"Disconnected\n";
		exit(-1);
	}
	if (recv_result != sf::Socket::NotReady)
		if (received > 0) process_data(net_buf, received);


	MapManger.Draw();
	


	avatar.draw();
	for (auto& pl : players) pl.second.draw();
	sf::Text text;
	text.setFont(g_font);
	char buf[100];
	sprintf_s(buf, "(%d, %d)", avatar.m_x, avatar.m_y);
	text.setString(buf);
	g_window->draw(text);


	UImanger->Draw();
}

void send_packet(void *packet)
{
	unsigned char *p = reinterpret_cast<unsigned char *>(packet);
	size_t sent = 0;
	s_socket.send(packet, MAKEWORD(p[0],p[1]), sent);
}



int main()
{
	std::string id = "";
	do {
		std::cout << "아이디를 입력해주세요" << std::endl;
		std::cin >> id;
	} while (id.size() > 20);

	wcout.imbue(locale("korean"));
	sf::Socket::Status status = s_socket.connect("127.0.0.1", PORT_NUM);
	s_socket.setBlocking(false);

	if (status != sf::Socket::Done) {
		wcout << L"서버와 연결할 수 없습니다.\n";
		exit(-1);
	}



	client_initialize();
	CS_LOGIN_PACKET p;
	p.size = sizeof(p);
	memcpy(p.userid, id.c_str(), NAME_SIZE);
	p.type = CS_LOGIN;

   	string player_name{ "ID:" };
	player_name += id;
		
	strcpy_s(p.name, player_name.c_str());
	send_packet(&p);
	avatar.set_name(p.name);

	sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "2D CLIENT");
	g_window = &window;
	UImanger = new UIManager(g_window);

	auto pretime = std::chrono::system_clock::now();
	while (window.isOpen())
	{

		auto nowtime = std::chrono::system_clock::now();
		// 경과 시간 계산
		 elapsedTime = nowtime - pretime;
		 pretime = nowtime;
		 

		 //텔레포트 보내기
		 if (UImanger->btelselectmode) {
			 CS_TELEPORT_PACKET p;
			 p.size = sizeof(CS_TELEPORT_PACKET);
			 p.type = CS_TELEPORT;
			 send_packet(&p);
			 UImanger->btelselectmode = false;
		 }
		 

		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();

			if (event.type == sf::Event::MouseMoved) {
				sf::Vector2i localPosition = sf::Mouse::getPosition(window);
				UImanger->MovingUI(localPosition);
				
			}

			if (event.type == sf::Event::MouseButtonPressed) {
				switch (event.key.code)
				{
				case sf::Mouse::Left: 
					sf::Vector2i localPosition = sf::Mouse::getPosition(window);
					UImanger->ClickUI(localPosition);
					break;

			
				}

			}
			if (event.type == sf::Event::TextEntered)
			{
				
				WCHAR temp = event.text.unicode;
				if (UImanger->bChattingmode) {
					if (temp == 8 && !UImanger->chatcontent.empty())
						UImanger->chatcontent.pop_back();
					else if (temp == 13) {
						if (UImanger->chatcontent.size()<1) {
							UImanger->bChattingmode = false;
						}
						else {
							UImanger->chatcontent += L"\0";
							CS_CHAT_PACKET p;
							memset(&p, 0, sizeof(CS_CHAT_PACKET));
							p.type = CS_CHAT;
							p.size = sizeof(CS_CHAT_PACKET);
							int i = 0;
							for (auto chr : UImanger->chatcontent) {
								p.mess[i++] = chr;

							}
							p.mess[i] = '\0';
							std::wcout << UImanger->chatcontent << std::endl;
							avatar.set_chat(p.mess);

							send_packet(&p);
							UImanger->chatcontent = L"";
						}
					}
					else if(UImanger->chatcontent.length()<CHAT_SIZE){
						UImanger->chatcontent += temp;
					}

				}
			

			}
		

			if (event.type == sf::Event::KeyPressed) {

				int direction = -1;
				switch (event.key.code) {

				case sf::Keyboard::Left:
					avatar.m_dir = -1.f;
					direction = 2;
					avatar.m_state = WALK;
					break;
				case sf::Keyboard::Right:
					avatar.m_dir = 1.f;
					direction = 3;
					avatar.m_state = WALK;

					break;
				case sf::Keyboard::Up:
					avatar.m_state = WALK;
					direction = 0;
					break;
				case sf::Keyboard::Down:
					avatar.m_state = WALK;
					direction = 1;
					break;
				case sf::Keyboard::LControl:
					CS_ATTACK_PACKET p;
					p.size = sizeof(p);
					p.type = CS_ATTACK;
					send_packet(&p);
					avatar.m_state = ATTACK;
					break;

				case sf::Keyboard::Escape:
					window.close();
					break;
				}

				

				if (-1 != direction) {
					CS_MOVE_PACKET p;
					p.size = sizeof(p);
					p.type = CS_MOVE;
					p.direction = direction;
					send_packet(&p);
				}

			}
		}

		window.clear();

		client_main();
		window.display();
	}
	client_finish();

	return 0;
}