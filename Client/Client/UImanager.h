#pragma once
#include <SFML/Graphics.hpp>
#include <iostream>	
#include <thread>
#include <windows.h>
#include <conio.h>

class UIManager {
public:
	enum state {
		unselect,
		select,
		stateend
	};
	int w_window_height = 1050;
	int w_window_width = 1050;
	sf::Texture* m_chat;
	sf::Sprite m_chatimg;

	sf::Texture* m_chatbackground;
	sf::Sprite m_chatbackgroundimg;

	sf::Texture* m_telbtn[2];
	sf::Sprite m_telbtnimg[2];
	bool telselect = false;
	bool btelselectmode = false;

	sf::RenderWindow* m_pwindow;
	sf::Font g_font;


	bool bChattingmode = false;
	sf::Text chatcontent_display;

	static std::wstring chatcontent;

	UIManager(sf::RenderWindow* g_window) {
		m_pwindow = g_window;

		if (false == g_font.loadFromFile("hangle.ttf")) {
			std::cout << "Font Loading Error!\n";
			exit(-1);
		}

		m_chat = new sf::Texture;
		m_chat->loadFromFile("Resource/UI/chatbutton.png");
		
		m_chatimg.setTexture(*m_chat);
		m_chatimg.setTextureRect(sf::IntRect(0, 0, 1024, 512));
		m_chatimg.setScale(0.3f, 0.3f);
		m_chatimg.setPosition(0, w_window_height - 200);

		m_chatbackground = new sf::Texture;
		m_chatbackground->loadFromFile("Resource/UI/chatbackground.png");
		m_chatbackgroundimg.setTexture(*m_chatbackground);
		m_chatbackgroundimg.setTextureRect(sf::IntRect(0, 0, 512, 512));
		m_chatbackgroundimg.setScale(1.7f, 0.3f);
		m_chatbackgroundimg.setPosition(140, w_window_height - 150);

		m_telbtn[0] = new sf::Texture;
		m_telbtn[0]->loadFromFile("Resource/UI/selecttelbtn.png");
		m_telbtnimg[0].setTexture(*m_telbtn[0]);
		m_telbtnimg[0].setTextureRect(sf::IntRect(0, 0, 512, 512));
		m_telbtnimg[0].setScale(0.3f, 0.3f);
		m_telbtnimg[0].setPosition(w_window_width - 170,  -5);

		m_telbtn[1] = new sf::Texture;
		m_telbtn[1]->loadFromFile("Resource/UI/telbtn.png");
		m_telbtnimg[1].setTexture(*m_telbtn[1]);
		m_telbtnimg[1].setTextureRect(sf::IntRect(0, 0, 512, 512));
		m_telbtnimg[1].setScale(0.3f, 0.3f);
		m_telbtnimg[1].setPosition(w_window_width - 170, -5);


	}

	void MovingUI(sf::Vector2i mousecoord) {
		//ChatButton
		bool ischatcursh = false;
		if (0 < mousecoord.x&& mousecoord.x < 150) {
			if (920 < mousecoord.y &&mousecoord.y < 1000) {
				ischatcursh = true;
			}
		}
		if (ischatcursh) {
			m_chatimg.setScale(0.32f, 0.32f);
		}
		else {
			m_chatimg.setScale(0.3f, 0.3f);
		}
		
		bool isteleportcursh = false;

		if (890 < mousecoord.x && mousecoord.x < 1050) {
			if (0 < mousecoord.y && mousecoord.y < 150) {
				isteleportcursh = true;
			}
		}
		if (isteleportcursh) {
			telselect = true;
		}
		else {
			telselect = false;
		}
	}
		
	void ClickUI(sf::Vector2i mousecoord) {

		//ChatButton
		bool ischatcursh = false;
		if (0 < mousecoord.x && mousecoord.x < 150) {
			if (920 < mousecoord.y && mousecoord.y < 1000) {
				ischatcursh = true;
			}
		}
		if (ischatcursh && bChattingmode) {
		
		}
		else if(ischatcursh && !bChattingmode){
			chatcontent = L"";
			bChattingmode = true;
		}
		else {
			chatcontent = L"";
			bChattingmode = false;
		}


		bool isteleportcursh = false;

		if (890 < mousecoord.x && mousecoord.x < 1050) {
			if (0 < mousecoord.y && mousecoord.y < 150) {
				isteleportcursh = true;
			}
		}
		if (isteleportcursh) {
			btelselectmode = true;
		}
		else {
			btelselectmode = false;
		}


	
	}


	void Draw() {
		if (bChattingmode) {
			
			if (chatcontent.empty()) {
				chatcontent_display.setString(L"보낼 메세지를 입력해주세요");
				chatcontent_display.setStyle(sf::Text::StrikeThrough);
			}
			else {
				
				chatcontent_display.setString(chatcontent);
				chatcontent_display.setStyle(sf::Text::Italic);

			}
			m_pwindow->draw(m_chatbackgroundimg);
			chatcontent_display.setFont(g_font);
			chatcontent_display.setFillColor(sf::Color(0, 0, 0));
			chatcontent_display.setPosition(200, w_window_height-100);
			m_pwindow->draw(chatcontent_display);
		}
		m_pwindow->draw(m_chatimg);


		if (telselect) {
			m_pwindow->draw(m_telbtnimg[unselect]);
		}
		else {
			m_pwindow->draw(m_telbtnimg[select]);
		}

	}

	

	
};
