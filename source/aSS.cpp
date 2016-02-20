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
#include <limits>

#include <tui.h>
#include <armadillo>

#include "curses.hpp"
#include "config.hpp"
#include "game_data.hpp"
#include "exceptions.hpp"
#include "quickscope_wrapper.hpp"


using namespace std;
using namespace arma;


#define BACKSPACE '\x08'
#define TAB '\x09'
#define CARRIAGE_RETURN '\x0d'

#define COLOR_PAIR_BLUE 1
#define COLOR_PAIR_RED 2
#define COLOR_PAIR_GREEN 3
#define COLOR_PAIR_WHITE 4


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


int main(int argc, const char * const * argv) {
	const auto options = parse_options(argc, argv);
	if(!options.first)
		return options.second;
	const auto config = options.first.value();

	initscr();
	quickscope_wrapper _endwin{[]() { endwin(); }};
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

	window_p main_screen(newwin(config.screen_height, config.screen_width, 0, 0));

	scrollok(main_screen.get(), true);
	clearok(main_screen.get(), true);
	for(const string & str : {"apoSimpleSmart"}) {
		mvwaddstr(main_screen.get(), config.screen_height / 2, (config.screen_width - str.size()) / 2, str.c_str());
		wrefresh(main_screen.get());
		bool broke = false;
		for(auto i = 0u; i < config.screen_height / 2 + 1; ++i) {
			if(getch() != ERR) {
				broke = true;
				break;
			}
			wscrl(main_screen.get(), 1);
			wrefresh(main_screen.get());
		}
		if(broke || getch() != ERR) {
			wclear(main_screen.get());
			wrefresh(main_screen.get());
			break;
		}
	}

	nocbreak();
	game_data global_data = load_game_data_from_file();

	bool shall_keep_going = true;
	while(shall_keep_going)
		switch(const int val = display_mainscreen(main_screen.get(), config.put_apo_in_screens)) {
			case mainscreen_selection::start:
				play_game(main_screen.get(), global_data.highscore);
				break;
			case mainscreen_selection::tutorial:
				wclear(main_screen.get());
				display_tutorialscreen(main_screen.get());
				wclear(main_screen.get());
				break;
			case mainscreen_selection::quit:
				shall_keep_going = false;
				break;
			case mainscreen_selection::credits:
				wclear(main_screen.get());
				display_creditsscreen(main_screen.get(), config.put_apo_in_screens);
				wclear(main_screen.get());
				break;
			case mainscreen_selection::options:
				wclear(main_screen.get());
				global_data.name = display_optionsscreen(main_screen.get(), global_data.name);
				curs_set(0);
				wclear(main_screen.get());

				save_game_data_to_file(global_data);
				break;
			case mainscreen_selection::highscore:
				wclear(main_screen.get());
				display_highscorescreen(main_screen.get(), global_data.highscore);
				wclear(main_screen.get());
				break;
			default:
				crash_report();
				throw simplesmart_exception("Wrong value returned by display_mainscreen(); value returned: " + to_string(val) + "; expected: 0..5");
		}
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

#define FOR_ALL_WINDOWS(func)         \
	func(start_button_window.get());    \
	func(tutorial_button_window.get()); \
	func(quit_button_window.get());     \
	func(credits_button_window.get());  \
	func(options_button_window.get());  \
	func(highscore_button_window.get());
#define FOR_ALL_WINDOWS_ARG(func, ...)             \
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
				int mouseX, mouseY;

				wmouse_position(start_button_window.get(), &mouseX, &mouseY);
				if(mouseX != -1 && mouseY != -1) {
					toret = mainscreen_selection::start;
					mvwprintw(parent_window, toret, 0, "%d %d\t", mouseX, mouseY);
				} else {
					wmouse_position(tutorial_button_window.get(), &mouseX, &mouseY);
					if(mouseX != -1 && mouseY != -1) {
						toret = mainscreen_selection::tutorial;
						mvwprintw(parent_window, toret, 0, "%d %d\t", mouseX, mouseY);
					} else {
						wmouse_position(quit_button_window.get(), &mouseX, &mouseY);
						if(mouseX != -1 && mouseY != -1) {
							toret = mainscreen_selection::quit;
							mvwprintw(parent_window, toret, 0, "%d %d\t", mouseX, mouseY);
						} else {
							wmouse_position(credits_button_window.get(), &mouseX, &mouseY);
							if(mouseX != -1 && mouseY != -1) {
								toret = mainscreen_selection::credits;
								mvwprintw(parent_window, toret, 0, "%d %d\t", mouseX, mouseY);
							} else {
								wmouse_position(options_button_window.get(), &mouseX, &mouseY);
								if(mouseX != -1 && mouseY != -1) {
									toret = mainscreen_selection::options;
									mvwprintw(parent_window, toret, 0, "%d %d\t", mouseX, mouseY);
								} else {
									wmouse_position(highscore_button_window.get(), &mouseX, &mouseY);
									if(mouseX != -1 && mouseY != -1) {
										toret = mainscreen_selection::highscore;
										mvwprintw(parent_window, toret, 0, "%d %d\t", mouseX, mouseY);
									}
								}
							}
						}
					}
				}
				wrefresh(parent_window);
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
	window_p menu_button_window(derwin(parent_window, 3, 6, maxY - 3, maxX - 6));
	window_p bigstring_message_window(derwin(parent_window, 8, 42, (maxY - 8) / 2, (maxX - 42) / 2));
	window_p credit_apo_message_window(derwin(parent_window, 3, 35, 0, (maxX - 35) / 2));
	window_p credit_me_message_window(derwin(parent_window, 3, 48, maxY - 3, (maxX - 48) / 2));
	window_p credit_cereal_message_window(derwin(parent_window, 3, 31, 3, (maxX - 31) / 2));
	touchwin(parent_window);
	wrefresh(parent_window);

	wborder(menu_button_window.get(), ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE, ACS_ULCORNER, ACS_URCORNER, ACS_LLCORNER, ACS_LRCORNER);
	mvwaddstr(menu_button_window.get(), 1, 1, "Menu");
	mvwchgat(menu_button_window.get(), 1, 1, 1, A_BOLD, 0, nullptr);
	wrefresh(menu_button_window.get());

	if(put_apo_in) {
		mvwaddstr(bigstring_message_window.get(), 0, 0, R"(  __   _   __       __           //@@\\   )");
		mvwaddstr(bigstring_message_window.get(), 1, 0, R"(//  \\|_|//  \\/ \//  \\   __   _||  ||   )");
		mvwaddstr(bigstring_message_window.get(), 2, 0, R"(||     _ ||  ||| |||  ||\//  \\| \\__//_  )");
		mvwaddstr(bigstring_message_window.get(), 3, 0, R"(\\__  | |\\__//\_|\\__//|||  ||| |//___ \\)");
		mvwaddstr(bigstring_message_window.get(), 4, 0, R"(    \\| |  //\\  |//\\  |\\__//| |||___\||)");
		mvwaddstr(bigstring_message_window.get(), 5, 0, R"(    ||| | //  \\ |/  \\ |      | |||      )");
		mvwaddstr(bigstring_message_window.get(), 6, 0, R"(\\__//|_|//    \|/    \\|      \_\\\____//)");
		mvwaddstr(bigstring_message_window.get(), 0, 35, "\304\304");
	} else {
		mvwaddstr(bigstring_message_window.get(), 0, 0, R"(  __   _                                  )");
		mvwaddstr(bigstring_message_window.get(), 1, 0, R"(//  \\|_|                  __   _         )");
		mvwaddstr(bigstring_message_window.get(), 2, 0, R"(||     _                \//  \\| |  ____  )");
		mvwaddstr(bigstring_message_window.get(), 3, 0, R"(\\__  | |               |||  ||| |//___ \\)");
		mvwaddstr(bigstring_message_window.get(), 4, 0, R"(    \\| |  //\\   //\\  |\\__//| |||___\||)");
		mvwaddstr(bigstring_message_window.get(), 5, 0, R"(    ||| | //  \\ //  \\ |      | |||      )");
		mvwaddstr(bigstring_message_window.get(), 6, 0, R"(\\__//|_|//    \|/    \\|      \_\\\____//)");
	}
	wrefresh(bigstring_message_window.get());

	mvwaddstr(credit_apo_message_window.get(), 0, 0, "Devs at Apo-Games for original game");
	mvwaddstr(credit_apo_message_window.get(), 1, 0, "        http://goo.gl/QKqIK1       ");
	wrefresh(credit_apo_message_window.get());

	mvwaddstr(credit_me_message_window.get(), 0, 0, "                 Repo at GitHub                 ");
	mvwaddstr(credit_me_message_window.get(), 1, 0, "https://github.com/nabijaczleweli/apoSimpleSmart");
	wrefresh(credit_me_message_window.get());

	mvwaddstr(credit_cereal_message_window.get(), 0, 0, "       USCiLab for cereal      ");
	mvwaddstr(credit_cereal_message_window.get(), 1, 0, "http://uscilab.github.io/cereal");
	wrefresh(credit_cereal_message_window.get());


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
				int mouseX, mouseY;

				wmouse_position(menu_button_window.get(), &mouseX, &mouseY);
				if(mouseX != -1 && mouseY != -1)
					clicked = true;
				break;
		}
	nodelay(menu_button_window.get(), true);
	mousemask(0, nullptr);
}

string display_optionsscreen(WINDOW * parent_window, const string & name) {
	int maxX, maxY;
	getmaxyx(parent_window, maxY, maxX);
	window_p menu_button_window(derwin(parent_window, 3, 6, maxY - 3, maxX - 6));
	window_p bigstring_message_window(derwin(parent_window, 7, 21, (maxY - 7) / 2, (maxX - 21) / 2));
	window_p name_editbox_window(derwin(parent_window, 3, game_data::max_name_length + 2, 0, (maxX - (game_data::max_name_length + 2)) / 2));
	touchwin(parent_window);
	wrefresh(parent_window);

	wborder(menu_button_window.get(), ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE, ACS_ULCORNER, ACS_URCORNER, ACS_LLCORNER, ACS_LRCORNER);
	mvwaddstr(menu_button_window.get(), 1, 1, "Menu");
	mvwchgat(menu_button_window.get(), 1, 1, 1, A_BOLD, 0, nullptr);
	wrefresh(menu_button_window.get());
	mvwaddstr(bigstring_message_window.get(), 0, 0, R"(           __        )");
	mvwaddstr(bigstring_message_window.get(), 1, 0, R"(        \//  \\      )");
	mvwaddstr(bigstring_message_window.get(), 2, 0, R"(  __    |||  ||  __  )");
	mvwaddstr(bigstring_message_window.get(), 3, 0, R"(//  \\/ |\\__////  \\)");
	mvwaddstr(bigstring_message_window.get(), 4, 0, R"(||  ||| |      ||  ||)");
	mvwaddstr(bigstring_message_window.get(), 5, 0, R"(\\__//\_|      \\__//)");
	wrefresh(bigstring_message_window.get());
	wborder(name_editbox_window.get(), ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE, ACS_ULCORNER, ACS_URCORNER, ACS_LLCORNER, ACS_LRCORNER);
	mvwaddstr(name_editbox_window.get(), 0, 1, "Name");
	wrefresh(name_editbox_window.get());

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
					int mouseX, mouseY;

					wmouse_position(menu_button_window.get(), &mouseX, &mouseY);
					if(mouseX != -1 && mouseY != -1)
						exited = true;
					break;
			}
		} else if(selected == 1) {
			if(maxX == -1 || maxY == -1) {
				curs_set(1);
				getmaxyx(name_editbox_window.get(), maxY, maxX);
				wmove(name_editbox_window.get(), 1, 1);
				for(int i = 0; i < maxX - 2; ++i)
					waddch(name_editbox_window.get(), ' ');
				mvwaddstr(name_editbox_window.get(), 1, 1, newname.c_str());
				wrefresh(name_editbox_window.get());
			}
			const char c = wgetch(name_editbox_window.get());
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
					wprintw(name_editbox_window.get(), "%c %c", BACKSPACE, BACKSPACE);
					wrefresh(name_editbox_window.get());
				}
			} else if(newname.size() < game_data::max_name_length) {
				newname.push_back(c);
				waddch(name_editbox_window.get(), c);
				wrefresh(name_editbox_window.get());
			}
		}
	}
	nodelay(menu_button_window.get(), true);
	nodelay(bigstring_message_window.get(), true);
	nodelay(name_editbox_window.get(), true);
	mousemask(0, nullptr);

	return newname;
}

void display_tutorialscreen(WINDOW * parent_window) {
	int maxX, maxY;
	getmaxyx(parent_window, maxY, maxX);
	window_p menu_button_window(derwin(parent_window, 3, 6, maxY - 3, maxX - 6));
	window_p moving_message_window(derwin(parent_window, 2, 37, 1, (maxX - 38) / 2));
	window_p clearing_message_window(derwin(parent_window, 2, 47, 3, (maxX - 48) / 2));
	window_p direction_message_window(derwin(parent_window, 4, 33, maxY - 4, (maxX - 33) / 2));
	window_p bigstring_message_window(derwin(parent_window, 8, 20, (maxY - 8) / 2, (maxX - 20) / 2));
	touchwin(parent_window);
	wrefresh(parent_window);

	wborder(menu_button_window.get(), ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE, ACS_ULCORNER, ACS_URCORNER, ACS_LLCORNER, ACS_LRCORNER);
	mvwaddstr(menu_button_window.get(), 1, 1, "Menu");
	mvwchgat(menu_button_window.get(), 1, 1, 1, A_BOLD, 0, nullptr);
	wrefresh(menu_button_window.get());
	mvwaddstr(bigstring_message_window.get(), 0, 0, R"(          __    __  )");
	mvwaddstr(bigstring_message_window.get(), 1, 0, R"(        //  \\//  \\)");
	mvwaddstr(bigstring_message_window.get(), 2, 0, R"(        ||    ||    )");
	mvwaddstr(bigstring_message_window.get(), 3, 0, R"(  __    \\__  \\__  )");
	mvwaddstr(bigstring_message_window.get(), 4, 0, R"(//  \\/     \\    \\)");
	mvwaddstr(bigstring_message_window.get(), 5, 0, R"(||  |||     ||    ||)");
	mvwaddstr(bigstring_message_window.get(), 6, 0, R"(\\__//\_\\__//\\__//)");
	wrefresh(bigstring_message_window.get());
	mvwaddstr(moving_message_window.get(), 0, 0, "  Start moving by touching on a piece");
	mvwaddch(moving_message_window.get(), 0, 0, right_pointing_moving_thing | COLOR_PAIR(COLOR_PAIR_WHITE));
	wrefresh(moving_message_window.get());
	mvwaddstr(clearing_message_window.get(), 0, 0, "   Clear all colored pieces to finish the level");
	mvwaddch(clearing_message_window.get(), 0, 0, up_pointing_moving_thing | COLOR_PAIR(COLOR_PAIR_BLUE));
	mvwaddch(clearing_message_window.get(), 0, 1, left_pointing_moving_thing | COLOR_PAIR(COLOR_PAIR_RED));
	wrefresh(clearing_message_window.get());
	mvwaddstr(direction_message_window.get(), 0, 0, "                                 ");
	mvwaddstr(direction_message_window.get(), 1, 0, " Whenever a new piece is reached ");
	mvwaddstr(direction_message_window.get(), 2, 0, "movement changes to its direction");
	mvwaddch(direction_message_window.get(), 0, 15, down_pointing_moving_thing | COLOR_PAIR(COLOR_PAIR_WHITE));
	mvwaddch(direction_message_window.get(), 0, 16, up_pointing_moving_thing | COLOR_PAIR(COLOR_PAIR_GREEN));
	wrefresh(direction_message_window.get());

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
				int mouseX, mouseY;

				wmouse_position(menu_button_window.get(), &mouseX, &mouseY);
				if(mouseX != -1 && mouseY != -1)
					clicked = true;
				break;
		}
	nodelay(menu_button_window.get(), true);
	nodelay(bigstring_message_window.get(), true);
	nodelay(moving_message_window.get(), true);
	nodelay(clearing_message_window.get(), true);
	mousemask(0, nullptr);
}

void display_highscorescreen(WINDOW * parent_window, const vector<high_data> & highscores) {
	static const auto maximal_score_width = to_string(numeric_limits<decltype(highscores[0].score)>::max() / 4).size();
	static const auto maximal_whole_width =
	    game_data::max_name_length + 1 + maximal_score_width + 1 + to_string(numeric_limits<decltype(highscores[0].level)>::max() / 4).size();

	int maxX, maxY;
	getmaxyx(parent_window, maxY, maxX);
	window_p menu_button_window(derwin(parent_window, 3, 6, maxY - 3, maxX - 6));
	window_p description_message_window(derwin(parent_window, 2, maximal_whole_width, 0, (maxX - maximal_whole_width) / 2));
	window_p none_message_window(highscores.size() ? nullptr : derwin(parent_window, 2, 23, maxY - 2, (maxX - 23) / 2));
	vector<window_p> highscores_messages_window;
	for(auto i = 0u; i < highscores.size(); ++i)
		highscores_messages_window.emplace_back(derwin(parent_window, 1, maximal_whole_width, i + 1, (maxX - maximal_whole_width) / 2));
	touchwin(parent_window);
	wrefresh(parent_window);

	wborder(menu_button_window.get(), ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE, ACS_ULCORNER, ACS_URCORNER, ACS_LLCORNER, ACS_LRCORNER);
	mvwaddstr(menu_button_window.get(), 1, 1, "Menu");
	mvwchgat(menu_button_window.get(), 1, 1, 1, A_BOLD, 0, nullptr);
	wrefresh(description_message_window.get());
	mvwaddstr(description_message_window.get(), 0, 0, "Name");
	for(auto i = 4u; i < game_data::max_name_length; ++i)
		waddch(description_message_window.get(), ' ');
	waddstr(description_message_window.get(), "|Score");
	for(auto i = 5u; i < maximal_score_width; ++i)
		waddch(description_message_window.get(), ' ');
	waddstr(description_message_window.get(), "|Level");
	wrefresh(description_message_window.get());

	for(auto i = 0u; i < highscores.size(); ++i) {
		wmove(highscores_messages_window[i].get(), 0, 0);
		wprintw(highscores_messages_window[i].get(), "%.*s", highscores[i].name.size(), highscores[i].name.c_str());
		for(auto j = highscores[i].name.size(); j < game_data::max_name_length; ++j)
			waddch(highscores_messages_window[i].get(), ' ');
		wprintw(highscores_messages_window[i].get(), "|%u", highscores[i].score);
		for(auto j = to_string(highscores[i].score).size(); j < maximal_score_width; ++j)
			waddch(highscores_messages_window[i].get(), ' ');
		wprintw(highscores_messages_window[i].get(), "|%hu", highscores[i].level);
		wrefresh(highscores_messages_window[i].get());
	}
	if(!highscores.size()) {
		mvwaddstr(none_message_window.get(), 0, 0, "None yet! Go make some!");
		wrefresh(none_message_window.get());
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
				int mouseX, mouseY;

				wmouse_position(menu_button_window.get(), &mouseX, &mouseY);
				if(mouseX != -1 && mouseY != -1)
					clicked = true;
				break;
		}
	nodelay(menu_button_window.get(), true);
	nodelay(description_message_window.get(), true);
	if(!highscores.size())
		nodelay(none_message_window.get(), true);
	for(const auto & highscore_message_window : highscores_messages_window)
		nodelay(highscore_message_window.get(), true);
	mousemask(0, nullptr);
}

void play_game(WINDOW *, vector<high_data> &) {}
