// The MIT License (MIT)

// Copyright (c) 2014 nabijaczleweli

// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


#include <fstream>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cstdio>

#include <tui.h>
#include <armadillo>

#include "curses.hpp"
#include "config.hpp"
#include "game_data.hpp"
#include "exceptions.hpp"


using namespace std;
using namespace arma;


#define BACKSPACE '\x08'
#define TAB '\x09'
#define CARRIAGE_RETURN '\x0d'

#define COLOR_PAIR_BLUE 1
#define COLOR_PAIR_RED 2
#define COLOR_PAIR_GREEN 3
#define COLOR_PAIR_WHITE 4

#define NAME_MAX 40

enum mainscreen_selection : char { start, tutorial, quit, credits, options, highscore };

mainscreen_selection display_mainscreen(WINDOW * parent_window, const bool put_apo_in);
void display_creditsscreen(WINDOW * parent_window, const bool put_apo_in);
string display_optionsscreen(WINDOW * parent_window, const string & name);
void display_tutorialscreen(WINDOW * parent_window);
void display_highscorescreen(WINDOW * parent_window, const vector<high_data> & highscores);
void play_game(WINDOW * parent_window, vector<high_data> & highscores);


constexpr static const chtype right_pointing_moving_thing = ')';
constexpr static const chtype left_pointing_moving_thing  = 'C';
constexpr static const chtype up_pointing_moving_thing    = '^';
constexpr static const chtype down_pointing_moving_thing  = 'U';


int main(int, const char * const * argv) {
	const auto options = parse_options(argv);
	if(!options.first)
		return options.second;
	const auto config = options.first.value();

	initscr();
	halfdelay(1);
	curs_set(0);
	noecho();

	if(has_colors()) {
		start_color();
		init_pair(COLOR_PAIR_RED, COLOR_BLACK, COLOR_RED);
		init_pair(COLOR_PAIR_GREEN, COLOR_BLACK, COLOR_GREEN);
		init_pair(COLOR_PAIR_BLUE, COLOR_BLACK, COLOR_CYAN);
		init_pair(COLOR_PAIR_WHITE, COLOR_BLACK, COLOR_WHITE);
	}

	WINDOW * main_screen = newwin(config.screen_height, config.screen_width, 0, 0);

	scrollok(main_screen, true);
	clearok(main_screen, true);
	for(const string & str : {"apoSimpleSmart"}) {
		mvwaddstr(main_screen, config.screen_height / 2, (config.screen_width - str.size()) / 2, str.c_str());
		wrefresh(main_screen);
		bool broke = false;
		for(auto i = 0u; i < config.screen_height / 2 + 1; ++i) {
			if(getch() != ERR) {
				broke = true;
				break;
			}
			wscrl(main_screen, 1);
			wrefresh(main_screen);
		}
		if(broke || getch() != ERR) {
			wclear(main_screen);
			wrefresh(main_screen);
			break;
		}
	}

	nocbreak();
	game_data global_data = load_game_data_from_file();

	bool shall_keep_going = true;
	while(shall_keep_going)
		switch(const int val = display_mainscreen(main_screen, config.put_apo_in_screens)) {
			case mainscreen_selection::start:
				play_game(main_screen, global_data.highscore);
				break;
			case mainscreen_selection::tutorial:
				wclear(main_screen);
				display_tutorialscreen(main_screen);
				wclear(main_screen);
				break;
			case mainscreen_selection::quit:
				shall_keep_going = false;
				break;
			case mainscreen_selection::credits:
				wclear(main_screen);
				display_creditsscreen(main_screen, config.put_apo_in_screens);
				wclear(main_screen);
				break;
			case mainscreen_selection::options:
				wclear(main_screen);
				global_data.name = display_optionsscreen(main_screen, global_data.name);
				curs_set(0);
				wclear(main_screen);

				save_game_data_to_file(global_data);
				break;
			case mainscreen_selection::highscore:
				wclear(main_screen);
				display_highscorescreen(main_screen, global_data.highscore);
				wclear(main_screen);
				break;
			default:
				delwin(main_screen);
				main_screen = nullptr;
				endwin();
				crash_report();
				throw simplesmart_exception("Wrong value returned by display_mainscreen(); value returned: " + to_string(val) + "; expected: 0..5");
		}


	delwin(main_screen);
	main_screen = nullptr;
	endwin();
}


mainscreen_selection display_mainscreen(WINDOW * parent_window, const bool put_apo_in) {
	int maxX, maxY;
	getmaxyx(parent_window, maxY, maxX);
	window_p start_button_window(derwin(parent_window, 3, 7, 0, (maxX - 7) / 2));
	int tempX, tempY;
	getparyx(start_button_window.get(), tempY, tempX);
	window_p tutorial_button_window(derwin(parent_window, 3, 10, tempY + 3, (maxX - 10) / 2));
	window_p quit_button_window(derwin(parent_window, 3, 6, maxY - 3, maxX - 6));
	window_p credits_button_window(derwin(parent_window, 3, 9, maxY - 3, 0));
	window_p options_button_window(derwin(parent_window, 3, 9, maxY - 3, (maxX - 9) / 2));
	getparyx(options_button_window.get(), tempY, tempX);
	window_p highscore_button_window(derwin(parent_window, 3, 10, tempY - 3, (maxX - 11) / 2));
	window_p bigstring_message_window(derwin(parent_window, 8, 51, (maxY - 8) / 2, (maxX - 51) / 2));
	touchwin(parent_window);
	wrefresh(parent_window);

#define FOR_ALL_WINDOWS(func)   \
	func(start_button_window.get());    \
	func(tutorial_button_window.get()); \
	func(quit_button_window.get());     \
	func(credits_button_window.get());  \
	func(options_button_window.get());  \
	func(highscore_button_window.get());
#define FOR_ALL_WINDOWS_ARG(func, ...)       \
	func(start_button_window.get(), __VA_ARGS__);    \
	func(tutorial_button_window.get(), __VA_ARGS__); \
	func(quit_button_window.get(), __VA_ARGS__);     \
	func(credits_button_window.get(), __VA_ARGS__);  \
	func(options_button_window.get(), __VA_ARGS__);  \
	func(highscore_button_window.get(), __VA_ARGS__);

	FOR_ALL_WINDOWS_ARG(wborder, ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE, ACS_ULCORNER, ACS_URCORNER, ACS_LLCORNER, ACS_LRCORNER)
	mvwaddstr(start_button_window.get(), 1, 1, "Start");
	mvwaddstr(tutorial_button_window.get(), 1, 1, "Tutorial");
	mvwaddstr(quit_button_window.get(), 1, 1, "Quit");
	mvwaddstr(credits_button_window.get(), 1, 1, "Credits");
	mvwaddstr(options_button_window.get(), 1, 1, "Options");
	mvwaddstr(highscore_button_window.get(), 1, 1, "Higscore");
	FOR_ALL_WINDOWS_ARG(mvwchgat, 1, 1, 1, A_BOLD, 0, nullptr);
	FOR_ALL_WINDOWS(wrefresh);

	if(put_apo_in) {
		mvwaddstr(bigstring_message_window.get(), 0, 0, R"(  __    __        __     __   ___________          )");
		mvwaddstr(bigstring_message_window.get(), 1, 0, R"(//  \\//  \\/  \//  \\ //  \\|__   _____ \    _    )");
		mvwaddstr(bigstring_message_window.get(), 2, 0, R"(||    ||  |||  |||  || ||  ||   \ /     \| __| |__ )");
		mvwaddstr(bigstring_message_window.get(), 3, 0, R"(\\__  \\__//\_ |\\__// \\__//   | |       |__   __|)");
		mvwaddstr(bigstring_message_window.get(), 4, 0, R"(    \\  //\\   |/\\  //@@\\/    | |          | |   )");
		mvwaddstr(bigstring_message_window.get(), 5, 0, R"(    || //  \\ /|  \\ ||  |||    | |          | |   )");
		mvwaddstr(bigstring_message_window.get(), 6, 0, R"(\\__////    \|/    \\\\__//\_   |_|          \_\   )");
		mvwaddstr(bigstring_message_window.get(), 4, 23, "\304\304");
	} else {
		mvwaddstr(bigstring_message_window.get(), 0, 0, R"(  __                          ___________          )");
		mvwaddstr(bigstring_message_window.get(), 1, 0, R"(//  \\                       |__   _____ \    _    )");
		mvwaddstr(bigstring_message_window.get(), 2, 0, R"(||                              \ /     \| __| |__ )");
		mvwaddstr(bigstring_message_window.get(), 3, 0, R"(\\__                   __       | |       |__   __|)");
		mvwaddstr(bigstring_message_window.get(), 4, 0, R"(    \\  //\\   //\\  //  \\/    | |          | |   )");
		mvwaddstr(bigstring_message_window.get(), 5, 0, R"(    || //  \\ //  \\ ||  |||    | |          | |   )");
		mvwaddstr(bigstring_message_window.get(), 6, 0, R"(\\__////    \|/    \\\\__//\_   |_|          \_\   )");
	}
	wrefresh(bigstring_message_window.get());


	halfdelay(1);
	raw();
	nonl();
	cbreak();
	mainscreen_selection toret = static_cast<mainscreen_selection>(-1);
	mousemask(ALL_MOUSE_EVENTS, nullptr);
	while(toret == static_cast<mainscreen_selection>(-1)) {
		switch(wgetch(parent_window)) {
			case 's':
			case 'S':
				toret = mainscreen_selection::start;
				break;
			case 't':
			case 'T':
				toret = mainscreen_selection::tutorial;
				break;
			case 'q':
			case 'Q':
				toret = mainscreen_selection::quit;
				break;
			case 'c':
			case 'C':
				toret = mainscreen_selection::credits;
				break;
			case 'o':
			case 'O':
				toret = mainscreen_selection::options;
				break;
			case 'h':
			case 'H':
				toret = mainscreen_selection::highscore;
				break;
			case KEY_MOUSE:
				request_mouse_pos();
				mvwprintw(parent_window, 0, 0, "%d %d %d %d %d\t", Mouse_status.x, Mouse_status.y, Mouse_status.button[0], Mouse_status.button[1],
				          Mouse_status.button[2]);  //  DEBUG
				wrefresh(parent_window);            //  DEBUG
				int *mouseX = new int, *mouseY = new int;

				wmouse_position(start_button_window.get(), mouseX, mouseY);
				if(*mouseX != -1 && *mouseY != -1) {
					toret = mainscreen_selection::start;
					mvwprintw(parent_window, toret, 0, "%d %d\t", *mouseX, *mouseY);
				} else {
					wmouse_position(tutorial_button_window.get(), mouseX, mouseY);
					if(*mouseX != -1 && *mouseY != -1) {
						toret = mainscreen_selection::tutorial;
						mvwprintw(parent_window, toret, 0, "%d %d\t", *mouseX, *mouseY);
					} else {
						wmouse_position(quit_button_window.get(), mouseX, mouseY);
						if(*mouseX != -1 && *mouseY != -1) {
							toret = mainscreen_selection::quit;
							mvwprintw(parent_window, toret, 0, "%d %d\t", *mouseX, *mouseY);
						} else {
							wmouse_position(credits_button_window.get(), mouseX, mouseY);
							if(*mouseX != -1 && *mouseY != -1) {
								toret = mainscreen_selection::credits;
								mvwprintw(parent_window, toret, 0, "%d %d\t", *mouseX, *mouseY);
							} else {
								wmouse_position(options_button_window.get(), mouseX, mouseY);
								if(*mouseX != -1 && *mouseY != -1) {
									toret = mainscreen_selection::options;
									mvwprintw(parent_window, toret, 0, "%d %d\t", *mouseX, *mouseY);
								} else {
									wmouse_position(highscore_button_window.get(), mouseX, mouseY);
									if(*mouseX != -1 && *mouseY != -1) {
										toret = mainscreen_selection::highscore;
										mvwprintw(parent_window, toret, 0, "%d %d\t", *mouseX, *mouseY);
									}
								}
							}
						}
					}
				}
				wrefresh(parent_window);

				delete mouseX;
				delete mouseY;
				mouseX = nullptr;
				mouseY = nullptr;
				break;
		}
	}
	FOR_ALL_WINDOWS_ARG(nodelay, true);
	mousemask(0, nullptr);

#undef FOR_ALL_WINDOWS
#undef FOR_ALL_WINDOWS_ARG

	return toret;
}

void display_creditsscreen(WINDOW * parent_window, const bool put_apo_in) {
	int maxX, maxY;
	getmaxyx(parent_window, maxY, maxX);
	WINDOW *menu_button_window       = derwin(parent_window, 3, 6, maxY - 3, maxX - 6),
	       *bigstring_message_window = derwin(parent_window, 8, 42, (maxY - 8) / 2, (maxX - 42) / 2),
	       *credit_apo_message_window = derwin(parent_window, 3, 35, 0, (maxX - 35) / 2),
	       *credit_me_message_window = derwin(parent_window, 3, 33, maxY - 3, (maxX - 33) / 2),
	       *credit_bcl_message_window = derwin(parent_window, 3, 23, 3, (maxX - 23) / 2);
	touchwin(parent_window);
	wrefresh(parent_window);

	wborder(menu_button_window, ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE, ACS_ULCORNER, ACS_URCORNER, ACS_LLCORNER, ACS_LRCORNER);
	mvwaddstr(menu_button_window, 1, 1, "Menu");
	mvwchgat(menu_button_window, 1, 1, 1, A_BOLD, 0, nullptr);
	wrefresh(menu_button_window);

	if(put_apo_in) {
		mvwaddstr(bigstring_message_window, 0, 0, R"(  __   _   __       __           //@@\\   )");
		mvwaddstr(bigstring_message_window, 1, 0, R"(//  \\|_|//  \\/ \//  \\   __   _||  ||   )");
		mvwaddstr(bigstring_message_window, 2, 0, R"(||     _ ||  ||| |||  ||\//  \\| \\__//_  )");
		mvwaddstr(bigstring_message_window, 3, 0, R"(\\__  | |\\__//\_|\\__//|||  ||| |//___ \\)");
		mvwaddstr(bigstring_message_window, 4, 0, R"(    \\| |  //\\  |//\\  |\\__//| |||___\||)");
		mvwaddstr(bigstring_message_window, 5, 0, R"(    ||| | //  \\ |/  \\ |      | |||      )");
		mvwaddstr(bigstring_message_window, 6, 0, R"(\\__//|_|//    \|/    \\|      \_\\\____//)");
		mvwaddstr(bigstring_message_window, 0, 35, "\304\304");
	} else {
		mvwaddstr(bigstring_message_window, 0, 0, R"(  __   _                                  )");
		mvwaddstr(bigstring_message_window, 1, 0, R"(//  \\|_|                  __   _         )");
		mvwaddstr(bigstring_message_window, 2, 0, R"(||     _                \//  \\| |  ____  )");
		mvwaddstr(bigstring_message_window, 3, 0, R"(\\__  | |               |||  ||| |//___ \\)");
		mvwaddstr(bigstring_message_window, 4, 0, R"(    \\| |  //\\   //\\  |\\__//| |||___\||)");
		mvwaddstr(bigstring_message_window, 5, 0, R"(    ||| | //  \\ //  \\ |      | |||      )");
		mvwaddstr(bigstring_message_window, 6, 0, R"(\\__//|_|//    \|/    \\|      \_\\\____//)");
	}
	wrefresh(bigstring_message_window);

	mvwaddstr(credit_apo_message_window, 0, 0, "Devs at Apo-Games for original game");
	mvwaddstr(credit_apo_message_window, 1, 0, "        http://goo.gl/QKqIK1       ");
	wrefresh(credit_apo_message_window);

	mvwaddstr(credit_me_message_window, 0, 0, "Everything else by nabijaczleweli");
	mvwaddstr(credit_me_message_window, 1, 0, "https://github.com/nabijaczleweli");
	wrefresh(credit_me_message_window);

	mvwaddstr(credit_bcl_message_window, 0, 0, "Marcus Geelnard for bcl");
	mvwaddstr(credit_bcl_message_window, 1, 0, "  http://bcl.comli.eu  ");
	wrefresh(credit_bcl_message_window);


	halfdelay(1);
	raw();
	nonl();
	cbreak();
	mousemask(ALL_MOUSE_EVENTS, nullptr);
	bool clicked = false;
	while(!clicked)
		switch(wgetch(parent_window)) {
			case 'm':
			case 'M':
			case CARRIAGE_RETURN:
				clicked = true;
				break;
			case KEY_MOUSE:
				request_mouse_pos();
				int *mouseX = new int, *mouseY = new int;

				wmouse_position(menu_button_window, mouseX, mouseY);
				if(*mouseX != -1 && *mouseY != -1)
					clicked = true;

				delete mouseX;
				delete mouseY;
				mouseX = nullptr;
				mouseY = nullptr;

				break;
		}
	nodelay(menu_button_window, true);
	mousemask(0, nullptr);

	delwin(menu_button_window);
	delwin(bigstring_message_window);
	delwin(credit_apo_message_window);
	delwin(credit_me_message_window);
	delwin(credit_bcl_message_window);
	menu_button_window        = nullptr;
	bigstring_message_window  = nullptr;
	credit_apo_message_window = nullptr;
	credit_me_message_window  = nullptr;
	credit_bcl_message_window = nullptr;
}

string display_optionsscreen(WINDOW * parent_window, const string & name) {
	int maxX, maxY;
	getmaxyx(parent_window, maxY, maxX);
	WINDOW *menu_button_window       = derwin(parent_window, 3, 6, maxY - 3, maxX - 6),
	       *bigstring_message_window = derwin(parent_window, 7, 21, (maxY - 7) / 2, (maxX - 21) / 2),
	       *name_editbox_window = derwin(parent_window, 3, NAME_MAX + 2, 0, (maxX - (NAME_MAX + 2)) / 2);
	touchwin(parent_window);
	wrefresh(parent_window);

	wborder(menu_button_window, ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE, ACS_ULCORNER, ACS_URCORNER, ACS_LLCORNER, ACS_LRCORNER);
	mvwaddstr(menu_button_window, 1, 1, "Menu");
	mvwchgat(menu_button_window, 1, 1, 1, A_BOLD, 0, nullptr);
	wrefresh(menu_button_window);
	mvwaddstr(bigstring_message_window, 0, 0, R"(           __        )");
	mvwaddstr(bigstring_message_window, 1, 0, R"(        \//  \\      )");
	mvwaddstr(bigstring_message_window, 2, 0, R"(  __    |||  ||  __  )");
	mvwaddstr(bigstring_message_window, 3, 0, R"(//  \\/ |\\__////  \\)");
	mvwaddstr(bigstring_message_window, 4, 0, R"(||  ||| |      ||  ||)");
	mvwaddstr(bigstring_message_window, 5, 0, R"(\\__//\_|      \\__//)");
	wrefresh(bigstring_message_window);
	wborder(name_editbox_window, ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE, ACS_ULCORNER, ACS_URCORNER, ACS_LLCORNER, ACS_LRCORNER);
	mvwaddstr(name_editbox_window, 0, 1, "Name");
	wrefresh(name_editbox_window);

	maxX = -1;
	maxY = -1;

	halfdelay(1);
	raw();
	nonl();
	cbreak();
	mousemask(ALL_MOUSE_EVENTS, nullptr);
	bool exited    = false;
	int selected   = 1;
	string newname = name;
	while(!exited) {
		selected %= 2;
		if(!selected) {
			curs_set(0);
			switch(wgetch(parent_window)) {
				case 'm':
				case 'M':
				case CARRIAGE_RETURN:
					exited = true;
					break;
				case TAB:
					++selected;
					break;
				case KEY_MOUSE:
					request_mouse_pos();
					int *mouseX = new int, *mouseY = new int;

					wmouse_position(menu_button_window, mouseX, mouseY);
					if(*mouseX != -1 && *mouseY != -1)
						exited = true;

					delete mouseX;
					delete mouseY;
					mouseX = nullptr;
					mouseY = nullptr;
					break;
			}
		} else if(selected == 1) {
			if(maxX == -1 || maxY == -1) {
				curs_set(1);
				getmaxyx(name_editbox_window, maxY, maxX);
				wmove(name_editbox_window, 1, 1);
				for(int i = 0; i < maxX - 2; ++i)
					waddch(name_editbox_window, ' ');
				mvwaddstr(name_editbox_window, 1, 1, newname.c_str());
				wrefresh(name_editbox_window);
			}
			const char c = wgetch(name_editbox_window);
			if(c == ERR)
				continue;
			else if(c == KEY_ESC) {
				selected = 0;
				maxX     = -1;
				maxY     = -1;
				continue;
			} else if(c == TAB) {
				++selected;
				maxX = -1;
				maxY = -1;
				continue;
			} else if(c == CARRIAGE_RETURN) {
				maxX = -1;
				maxY = -1;
				if((selected = (selected + 1) % 2))
					selected = max(selected, 1);
				else
					exited = true;
			} else if(c == BACKSPACE) {
				if(newname.size()) {
					newname.pop_back();
					wprintw(name_editbox_window, "%c %c", BACKSPACE, BACKSPACE);
					wrefresh(name_editbox_window);
				}
			} else if(static_cast<int>(newname.size()) < NAME_MAX) {
				newname.push_back(c);
				waddch(name_editbox_window, c);
				wrefresh(name_editbox_window);
			}
		}
	}
	nodelay(menu_button_window, true);
	nodelay(bigstring_message_window, true);
	nodelay(name_editbox_window, true);
	mousemask(0, nullptr);

	delwin(menu_button_window);
	delwin(bigstring_message_window);
	delwin(name_editbox_window);
	menu_button_window       = nullptr;
	bigstring_message_window = nullptr;
	name_editbox_window      = nullptr;

	return newname;
}

void display_tutorialscreen(WINDOW * parent_window) {
	int maxX, maxY;
	getmaxyx(parent_window, maxY, maxX);
	WINDOW *menu_button_window = derwin(parent_window, 3, 6, maxY - 3, maxX - 6), *moving_message_window = derwin(parent_window, 2, 37, 1, (maxX - 38) / 2),
	       *clearing_message_window = derwin(parent_window, 2, 47, 3, (maxX - 48) / 2),
	       *direction_message_window = derwin(parent_window, 4, 33, maxY - 4, (maxX - 33) / 2),
	       *bigstring_message_window = derwin(parent_window, 8, 20, (maxY - 8) / 2, (maxX - 20) / 2);
	touchwin(parent_window);
	wrefresh(parent_window);

	wborder(menu_button_window, ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE, ACS_ULCORNER, ACS_URCORNER, ACS_LLCORNER, ACS_LRCORNER);
	mvwaddstr(menu_button_window, 1, 1, "Menu");
	mvwchgat(menu_button_window, 1, 1, 1, A_BOLD, 0, nullptr);
	wrefresh(menu_button_window);
	mvwaddstr(bigstring_message_window, 0, 0, R"(          __    __  )");
	mvwaddstr(bigstring_message_window, 1, 0, R"(        //  \\//  \\)");
	mvwaddstr(bigstring_message_window, 2, 0, R"(        ||    ||    )");
	mvwaddstr(bigstring_message_window, 3, 0, R"(  __    \\__  \\__  )");
	mvwaddstr(bigstring_message_window, 4, 0, R"(//  \\/     \\    \\)");
	mvwaddstr(bigstring_message_window, 5, 0, R"(||  |||     ||    ||)");
	mvwaddstr(bigstring_message_window, 6, 0, R"(\\__//\_\\__//\\__//)");
	wrefresh(bigstring_message_window);
	mvwaddstr(moving_message_window, 0, 0, "  Start moving by touching on a piece");
	mvwaddch(moving_message_window, 0, 0, right_pointing_moving_thing | COLOR_PAIR(COLOR_PAIR_WHITE));
	wrefresh(moving_message_window);
	mvwaddstr(clearing_message_window, 0, 0, "   Clear all colored pieces to finish the level");
	mvwaddch(clearing_message_window, 0, 0, up_pointing_moving_thing | COLOR_PAIR(COLOR_PAIR_BLUE));
	mvwaddch(clearing_message_window, 0, 1, left_pointing_moving_thing | COLOR_PAIR(COLOR_PAIR_RED));
	wrefresh(clearing_message_window);
	mvwaddstr(direction_message_window, 0, 0, "                                 ");
	mvwaddstr(direction_message_window, 1, 0, " Whenever a new piece is reached ");
	mvwaddstr(direction_message_window, 2, 0, "movement changes to its direction");
	mvwaddch(direction_message_window, 0, 15, down_pointing_moving_thing | COLOR_PAIR(COLOR_PAIR_WHITE));
	mvwaddch(direction_message_window, 0, 16, up_pointing_moving_thing | COLOR_PAIR(COLOR_PAIR_GREEN));
	wrefresh(direction_message_window);

	halfdelay(1);
	raw();
	nonl();
	cbreak();
	mousemask(ALL_MOUSE_EVENTS, nullptr);
	bool clicked = false;
	while(!clicked)
		switch(wgetch(parent_window)) {
			case 'm':
			case 'M':
			case CARRIAGE_RETURN:
				clicked = true;
				break;
			case KEY_MOUSE:
				request_mouse_pos();
				int *mouseX = new int, *mouseY = new int;

				wmouse_position(menu_button_window, mouseX, mouseY);
				if(*mouseX != -1 && *mouseY != -1)
					clicked = true;

				delete mouseX;
				delete mouseY;
				mouseX = nullptr;
				mouseY = nullptr;

				break;
		}
	nodelay(menu_button_window, true);
	nodelay(bigstring_message_window, true);
	nodelay(moving_message_window, true);
	nodelay(clearing_message_window, true);
	mousemask(0, nullptr);

	delwin(menu_button_window);
	delwin(bigstring_message_window);
	delwin(moving_message_window);
	delwin(clearing_message_window);
	menu_button_window       = nullptr;
	bigstring_message_window = nullptr;
	moving_message_window    = nullptr;
	clearing_message_window  = nullptr;
}

void display_highscorescreen(WINDOW * parent_window, const vector<high_data> & highscores) {
	static const auto maximal_score_width = to_string(numeric_limits<uint32_t>::max() / 4).size();
	static const auto maximal_whole_width = NAME_MAX + 1 + maximal_score_width + 1 + to_string(numeric_limits<uint16_t>::max() / 4).size();

	int maxX, maxY;
	getmaxyx(parent_window, maxY, maxX);
	WINDOW *menu_button_window         = derwin(parent_window, 3, 6, maxY - 3, maxX - 6),
	       *description_message_window = derwin(parent_window, 2, maximal_whole_width, 0, (maxX - maximal_whole_width) / 2),
	       **highscores_messages_window = new WINDOW *[highscores.size()],
	       *none_message_window = highscores.size() ? nullptr : derwin(parent_window, 2, 23, maxY - 2, (maxX - 23) / 2);
	for(auto i = 0u; i < highscores.size(); ++i)
		highscores_messages_window[i] = derwin(parent_window, 1, maximal_whole_width, i + 1, (maxX - maximal_whole_width) / 2), touchwin(parent_window);
	wrefresh(parent_window);

	wborder(menu_button_window, ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE, ACS_ULCORNER, ACS_URCORNER, ACS_LLCORNER, ACS_LRCORNER);
	mvwaddstr(menu_button_window, 1, 1, "Menu");
	mvwchgat(menu_button_window, 1, 1, 1, A_BOLD, 0, nullptr);
	wrefresh(description_message_window);
	mvwaddstr(description_message_window, 0, 0, "Name");
	for(auto i = 4; i < NAME_MAX; ++i)
		waddch(description_message_window, ' ');
	waddstr(description_message_window, "|Score");
	for(auto i = 5u; i < maximal_score_width; ++i)
		waddch(description_message_window, ' ');
	waddstr(description_message_window, "|Level");
	wrefresh(description_message_window);

	for(auto i = 0u; i < highscores.size(); ++i) {
		wmove(highscores_messages_window[i], 0, 0);
		wprintw(highscores_messages_window[i], "%.*s", highscores[i].name.size(), highscores[i].name.c_str());
		for(auto j = highscores[i].name.size(); j < NAME_MAX; ++j)
			waddch(highscores_messages_window[i], ' ');
		wprintw(highscores_messages_window[i], "|%u", highscores[i].score);
		for(auto j = to_string(highscores[i].score).size(); j < maximal_score_width; ++j)
			waddch(highscores_messages_window[i], ' ');
		wprintw(highscores_messages_window[i], "|%hu", highscores[i].level);
		wrefresh(highscores_messages_window[i]);
	}
	if(!highscores.size()) {
		mvwaddstr(none_message_window, 0, 0, "None yet! Go make some!");
		wrefresh(none_message_window);
	}

	halfdelay(1);
	raw();
	nonl();
	cbreak();
	mousemask(ALL_MOUSE_EVENTS, nullptr);
	bool clicked = false;
	while(!clicked)
		switch(wgetch(parent_window)) {
			case 'm':
			case 'M':
			case CARRIAGE_RETURN:
				clicked = true;
				break;
			case KEY_MOUSE:
				request_mouse_pos();
				int *mouseX = new int, *mouseY = new int;

				wmouse_position(menu_button_window, mouseX, mouseY);
				if(*mouseX != -1 && *mouseY != -1)
					clicked = true;

				delete mouseX;
				delete mouseY;
				mouseX = nullptr;
				mouseY = nullptr;

				break;
		}
	nodelay(menu_button_window, true);
	nodelay(description_message_window, true);
	if(!highscores.size())
		nodelay(none_message_window, true);
	for(auto i = 0u; i < highscores.size(); ++i)
		nodelay(highscores_messages_window[i], true);
	mousemask(0, nullptr);

	delwin(menu_button_window);
	delwin(description_message_window);
	for(auto i = 0u; i < highscores.size(); ++i) {
		delwin(highscores_messages_window[i]);
		highscores_messages_window[i] = nullptr;
	}
	delete[] highscores_messages_window;
	if(!highscores.size()) {
		delwin(none_message_window);
		none_message_window = nullptr;
	}
	highscores_messages_window = nullptr;
	description_message_window = nullptr;
	menu_button_window         = nullptr;
}

void play_game(WINDOW * parent_window, vector<high_data> & highscores) {}
