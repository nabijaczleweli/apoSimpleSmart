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


#include <cstring>
#include <cstdlib>
#include <fstream>
#include <cstdio>
#include <random>
#include <limits>

#include <tui.h>
#include "Eigen/Core"
#include "seed11/seed_device.hpp"

#include "curses.hpp"
#include "config.hpp"
#include "game_data.hpp"
#include "exceptions.hpp"
#include "quickscope_wrapper.hpp"


using namespace std;
using namespace Eigen;


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
void play_game(WINDOW * parent_window, const ass_config & cfg, game_data & gd);


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

	game_data global_data = load_game_data_from_file();
	global_data.highscore.emplace_back(high_data{"bulerb", 40, 100});

	bool shall_keep_going = true;
	while(shall_keep_going)
		switch(const int val = display_mainscreen(main_screen.get(), config.put_apo_in_screens)) {
			case mainscreen_selection::start:
				play_game(main_screen.get(), config, global_data);
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


	raw();
	nonl();
	cbreak();
	mainscreen_selection toret = static_cast<mainscreen_selection>(-1);
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
		}
	}

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


	raw();
	nonl();
	cbreak();

	while(true) {
		const auto pressed = wgetch(parent_window);
		if(pressed == 'm' || pressed == 'M' || pressed == CARRIAGE_RETURN)
			break;
	}
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

	raw();
	nonl();
	cbreak();
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

	raw();
	nonl();
	cbreak();

	while(true) {
		const auto pressed = wgetch(parent_window);
		if(pressed == 'm' || pressed == 'M' || pressed == CARRIAGE_RETURN)
			break;
	}
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
	wrefresh(menu_button_window.get());

	wprintw(description_message_window.get(), ("%-" + to_string(game_data::max_name_length) + "s|%-" + to_string(maximal_score_width) + "s|%s").c_str(), "Name",
	        "Score", "Level");
	wrefresh(description_message_window.get());

	for(auto i = 0u; i < highscores.size(); ++i) {
		wmove(highscores_messages_window[i].get(), 0, 0);
		wprintw(highscores_messages_window[i].get(), ("%-" + to_string(game_data::max_name_length) + "s|%-" + to_string(maximal_score_width) + "u|%hu").c_str(),
		        highscores[i].name.c_str(), highscores[i].score, highscores[i].level);
		wrefresh(highscores_messages_window[i].get());
	}
	if(!highscores.size()) {
		mvwaddstr(none_message_window.get(), 0, 0, "None yet, go make some.");
		wrefresh(none_message_window.get());
	}

	raw();
	nonl();
	cbreak();

	while(true) {
		const auto pressed = wgetch(parent_window);
		if(pressed == 'm' || pressed == 'M' || pressed == CARRIAGE_RETURN)
			break;
	}
}

void play_game(WINDOW * parent_window, const ass_config & cfg, game_data & gd) {
	enum direction : char { up, right, down, left, nonexistant };
	enum colour : int { none, blue, red, green, white };

	struct cell {
		direction dir;
		colour col;
	};


	int maxX, maxY;
	getmaxyx(parent_window, maxY, maxX);

	window_p matrix_window(derwin(parent_window, cfg.matrix_height, cfg.matrix_width, (maxY - cfg.matrix_height) / 2, (maxX - cfg.matrix_width) / 2));
	touchwin(parent_window);
	wrefresh(parent_window);
	wclear(parent_window);


	Matrix<cell, Dynamic, Dynamic> cells(cfg.matrix_height, cfg.matrix_width);
	mt19937 random(seed11::seed_device{}());

	{
		uniform_int_distribution<short> direction_distro(direction::up, direction::left);
		discrete_distribution<int> colour_distro({80, 5, 5, 5, 5});
		for(auto y = 0u; y < cfg.matrix_height; ++y)
			for(auto x = 0u; x < cfg.matrix_height; ++x)
				cells(y, x) = {static_cast<direction>(direction_distro(random)), static_cast<colour>(colour_distro(random))};
	}


	raw();

	unsigned int selected_y = 0;
	unsigned int selected_x = 0;
	while(true) {
		{
			uniform_int_distribution<int> colour_distro(COLOR_PAIR_BLUE, COLOR_PAIR_WHITE);
			for(auto y = 0u; y < cfg.matrix_height; ++y)
				for(auto x = 0u; x < cfg.matrix_width; ++x) {
					const auto & cell = cells(y, x);
					switch(cell.dir) {
						case direction::up:
							mvwaddch(matrix_window.get(), y, x, up_pointing_moving_thing);
							break;
						case direction::right:
							mvwaddch(matrix_window.get(), y, x, right_pointing_moving_thing);
							break;
						case direction::down:
							mvwaddch(matrix_window.get(), y, x, down_pointing_moving_thing);
							break;
						case direction::left:
							mvwaddch(matrix_window.get(), y, x, left_pointing_moving_thing);
							break;
						case direction::nonexistant:
							mvwaddch(matrix_window.get(), y, x, ' ');
							break;
					}
					if(cell.col)
						mvwchgat(matrix_window.get(), y, x, 1, COLOR_PAIR(cell.col), 0, nullptr);
				}
		}
		mvwchgat(matrix_window.get(), selected_y, selected_x, 1, A_BOLD, 0, nullptr);
		wrefresh(matrix_window.get());

		int score   = 0;
		auto & cell = cells(selected_y, selected_x);
		switch(wgetch(parent_window)) {
			case 'W':
			case 'w':
				if(selected_y)
					--selected_y;
				break;
			case 'S':
			case 's':
				if(selected_y < cfg.matrix_height - 1)
					++selected_y;
				break;
			case 'D':
			case 'd':
				if(selected_x < cfg.matrix_width - 1)
					++selected_x;
				break;
			case 'A':
			case 'a':
				if(selected_x)
					--selected_x;
				break;
			case ';': {
				if(cell.col != colour::none)
					break;
				auto cur_y = selected_y;
				auto cur_x = selected_x;
				bool cont  = true;
				while(cont) {
					switch(cells(cur_y, cur_x).dir) {
						case direction::up:
							if(cur_y) {
								--cur_y;
								++score;
							} else
								cont = false;
							break;
						case direction::right:
							if(cur_x < cfg.matrix_width) {
								++cur_x;
								++score;
							} else
								cont = false;
							break;
						case direction::down:
							if(cur_y < cfg.matrix_height) {
								++cur_y;
								++score;
							} else
								cont = false;
							break;
						case direction::left:
							if(cur_x) {
								--cur_x;
								++score;
							} else
								cont = false;
							break;
						case direction::nonexistant:
							break;
					}
					out << cur_x << ' ' << cur_y << " : " << score << endl;
				}
			} break;
			case 'Q':
			case 'q':
				return;
		}
	}
}
