#pragma once
#include <SFML/Graphics.hpp>
#include <iostream>	
#include <thread>
#include <windows.h>
#include <conio.h>

class UIManager {
public:
	int w_window_height = 1050;
	int w_window_width = 1050;
	sf::Texture* m_chat;
	sf::Sprite m_chatimg;

	sf::Texture* m_chatbackground;
	sf::Sprite m_chatbackgroundimg;
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

	}

	

	
};
