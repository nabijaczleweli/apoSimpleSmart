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
#include <unordered_map>
#include <cstdlib>
#include <cstdio>

#include <tui.h>
#include <armadillo>
#include "bcl/src/huffman.h"
#include "bcl/src/lz.h"
#include "bcl/src/rice.h"
#include "bcl/src/rle.h"
#include "bcl/src/shannonfano.h"

#include "exceptions.hpp"

using namespace std;
using namespace arma;


#define BACKSPACE '\x08'
#define TAB '\x09'
#define CARRIAGE_RETURN '\x0d'

#define GAME_DATA__NAME_MAX 33
#define GAME_DATA__HIGHSCORE_MAX_AMOUNT 10

#define HIGH_DATA__SERIALIZATION_SIZE (sizeof(uint8_t) + GAME_DATA__NAME_MAX + sizeof(uint32_t) + sizeof(uint16_t))
#define GAME_DATA__SERIALIZATION_SIZE \
	(sizeof(uint8_t) + GAME_DATA__NAME_MAX + sizeof(uint8_t) + GAME_DATA__HIGHSCORE_MAX_AMOUNT * HIGH_DATA__SERIALIZATION_SIZE)

#define COLOR_PAIR_BLUE 1
#define COLOR_PAIR_RED 2
#define COLOR_PAIR_GREEN 3
#define COLOR_PAIR_WHITE 4

struct game_data;
struct high_data;
enum mainscreen_selection : char { start, tutorial, quit, credits, options, highscore };

mainscreen_selection display_mainscreen(WINDOW * parent_window, const bool put_apo_in);
void display_creditsscreen(WINDOW * parent_window, const bool put_apo_in);
string display_optionsscreen(WINDOW * parent_window, const string & name);
void display_tutorialscreen(WINDOW * parent_window);
void display_highscorescreen(WINDOW * parent_window, uint8_t amount_of_highscores, const high_data * const highscores);
void play_game(WINDOW * parent_window, high_data * const highscores);

// Broken g++ workaround.
template <class T>
constexpr inline string to_string(T val);
void crash_report();
unsigned long int filesize(const char * const filename);

enum compression_method : char { lz = 'l', flz = 'f', huffman = 'h', rice8 = '8', riceu8 = 'u', rle = 'r', sf = 's' };

ostream & operator<<(ostream & strm, compression_method method) {
	return strm << static_cast<char>(method);
}

game_data * load__game_data__from_file(game_data * const output_gd, const string & filename = "gd.dat");
void save__game_data__to_file(const game_data * const input_gd, compression_method method, const string & filename = "gd.dat");

struct high_data {
	uint8_t length_of_name;
	char name[GAME_DATA__NAME_MAX];
	uint32_t score;
	uint16_t level;
};

struct game_data {
	uint8_t length_of_name;
	char name[GAME_DATA__NAME_MAX];
	uint8_t amount_of_highscores;
	high_data highscore[GAME_DATA__HIGHSCORE_MAX_AMOUNT];

	game_data() {
		char * const dis = static_cast<char *>(static_cast<void *>(this));
		memset(dis, GAME_DATA__SERIALIZATION_SIZE, '\x41');
		length_of_name       = 0;
		amount_of_highscores = 0;
	}
};

constexpr static const chtype right_pointing_moving_thing = ')';
constexpr static const chtype left_pointing_moving_thing  = 'C';
constexpr static const chtype up_pointing_moving_thing    = '^';
constexpr static const chtype down_pointing_moving_thing  = 'U';


int main(int, const char * argv[]) {
	string configfilename = "simple_smart.cfg";
	for(unsigned int idx = 1; argv[idx]; ++idx) {
		const char * const arg    = argv[idx];
		const unsigned int arglen = strlen(arg);
		if(*arg == '-' && arg[1]) {
			if(arglen >= 13 && !memcmp(arg, "--configfile=", 13)) {
				if(arglen == 13) {
					cerr << *argv << ": error: missing filename after \'--configfile=\'\n";
					return 1;
				}
				configfilename = arg + 13;
			} else if(arglen >= 2 && !memcmp(arg, "-c", 2)) {
				if(arglen == 2) {
					if(!argv[idx + 1]) {
						cerr << *argv << ": error: missing filename after \'-c\'\n";
						return 1;
					}
					configfilename = argv[idx + 1];
					++idx;
				} else
					configfilename = arg + 2;
			} else if(arglen >= 6 && !memcmp(arg, "--help", 6)) {
				if(arglen == 6) {
					cout << "Usage: " << *argv << " [options]\n"
					                              "Options:\n"
					                              "  --help                    Display this information\n"
					                              "  --help-compression-format Display information about allowed compression mechanisms\n"
					                              "  --configfile=<file>       Use config file file; Default: simple_smart.cfg\n"
					                              "  -c <file>                 Use config file file; Default: simple_smart.cfg\n"
					                              "\n"
					                              "For bug reporting instructions, please see:\n"
					                              "<https://github.com/nabijaczleweli>.\n";
				} else if(arglen == 25 && !memcmp(arg + 6, "-compression-format", 19)) {
					cout << "Compression format list.\n"
					        "\n"
					        "\'"
					     << compression_method::lz << "\' means LZ_Compress for compression and LZ_Uncompress for decompression.\n"
					                                  "    It implies using the LZ algorithm.\n\n"
					                                  "\'"
					     << compression_method::flz << "\' means LZ_CompressFast for compression and LZ_Uncompress for decompression.\n"
					                                   "    It implies using the LZ algorithm. It uses "
					     << (GAME_DATA__SERIALIZATION_SIZE + 65536) * sizeof(unsigned int) << " Bytes more,\n"
					                                                                          "    but is WAY quicker. Also, \'tis the default one.\n\n"
					                                                                          "\'"
					     << compression_method::huffman << "\' means Huffman_Compress for compression and Huffman_Uncompress for decompression.\n"
					                                       "    It implies using the Huffman algorithm.\n\n"
					                                       "\'"
					     << compression_method::rice8 << "\' means Rice_Compress for compression and Rice_Uncompress for decompression.\n"
					                                     "    It implies using the Rice signed 8 bit algorithm.\n\n"
					                                     "\'"
					     << compression_method::riceu8 << "\' means Rice_Compress for compression and Rice_Uncompress for decompression.\n"
					                                      "    It implies using the Rice unsigned 8 bit algorithm.\n\n"
					                                      "\'"
					     << compression_method::rle << "\' means RLE_Compress for compression and RLE_Uncompress for decompression.\n"
					                                   "    It implies using the RLE algorithm.\n\n"
					                                   "\'"
					     << compression_method::sf << "\' means SF_Compress for compression and SF_Uncompress for decompression.\n"
					                                  "    It implies using the Shannon-Fano algorithm.\n";
				}
				return 0;
			}
		}
	}


	unsigned int matrix_width = 7, matrix_height = 7, screen_width = 80, screen_height = 25;
	bool put_apo_in_screens          = false;
	compression_method saving_method = compression_method::flz;
	ifstream configfile(configfilename);
	if(!configfile)
		ofstream(configfilename) << boolalpha << "Matrix height : " << matrix_height << "\n"
		                                                                                "Matrix width : "
		                         << matrix_width << "\n"
		                                            "Screen height : "
		                         << screen_height << "\n"
		                                             "Screen width : "
		                         << screen_width << "\n"
		                                            "Put \'apo\' in screens : "
		                         << put_apo_in_screens << "\n"
		                                                  "Possible values: LZ \'"
		                         << compression_method::lz << "\', Fast LZ \'" << compression_method::flz << "\', Huffman \'" << compression_method::huffman
		                         << "\', Rice 8bit \'" << compression_method::rice8 << "\', Rice unsigned 8bit \'" << compression_method::riceu8 << "\', RLE \'"
		                         << compression_method::rle << "\', SF \'" << compression_method::sf << "\'.\n"
		                                                                                                "Saving method : "
		                         << saving_method << '\n';
	else {
		unordered_map<string, string> values(6);
		string line;
		while(getline(configfile, line)) {
			if(line.front() == '#' || line.find(" : ") == string::npos)
				continue;
			while(isspace(line.front()))
				line = line.substr(1);
			if(line.front() == '#' || line.find(" : ") == string::npos)
				continue;
			while(isspace(line.back()))
				line.pop_back();
			if(line.find(" : ") == string::npos)
				continue;
			const size_t pos = line.find(" : ");
			values.emplace(line.substr(0, pos), line.substr(pos + 3));
		}

#define ERRORMESSAGE(nonwhat)                                                                                                                         \
	{                                                                                                                                                   \
		cerr << *argv << ": warning: non-" << nonwhat << " value in \"" << configfilename << "\" at key \"" << pr.first << "\" for value \"" << pr.second \
		     << "\" reverting to previous/default\n";                                                                                                     \
		was_error = true;                                                                                                                                 \
	}
#define READPROPERTY(idname, errormsg, charcondition, setwithcondition) \
	if(pr.first == idname) {                                              \
		bool bad = false;                                                   \
		for(const char & ch : pr.second)                                    \
			if(!charcondition(ch)) {                                          \
				ERRORMESSAGE(errormsg)                                          \
				bad = true;                                                     \
				break;                                                          \
			}                                                                 \
		if(bad)                                                             \
			continue;                                                         \
		setwithcondition                                                    \
	}
#define READBOOLEANPROPERTY(idname, varname)                                                       \
	if(pr.first == idname) {                                                                         \
		switch(pr.second.size()) {                                                                     \
			case 1:                                                                                      \
				switch(pr.second[0]) {                                                                     \
					case '0':                                                                                \
						varname = false;                                                                       \
						break;                                                                                 \
					case '1':                                                                                \
						varname = true;                                                                        \
						break;                                                                                 \
					default:                                                                                 \
						ERRORMESSAGE("boolean")                                                                \
				}                                                                                          \
				break;                                                                                     \
			case 2:                                                                                      \
				if(pr.second == "no")                                                                      \
					varname = false;                                                                         \
				else                                                                                       \
					ERRORMESSAGE("boolean")                                                                  \
				break;                                                                                     \
			case 3:                                                                                      \
				if(pr.second == "yes")                                                                     \
					varname = true;                                                                          \
				else                                                                                       \
					ERRORMESSAGE("boolean")                                                                  \
				break;                                                                                     \
			case 4:                                                                                      \
				if(pr.second == "true")                                                                    \
					varname = true;                                                                          \
				else                                                                                       \
					ERRORMESSAGE("boolean")                                                                  \
				break;                                                                                     \
			case 5:                                                                                      \
				if(pr.second == "false")                                                                   \
					varname = false;                                                                         \
				else                                                                                       \
					ERRORMESSAGE("boolean")                                                                  \
				break;                                                                                     \
			default:                                                                                     \
				ERRORMESSAGE("lengthy-enough (1..5) (actual length: " + to_string(pr.second.size()) + ")") \
		}                                                                                              \
	}

		bool was_error = false;
		for(const auto & pr : values) {
			READPROPERTY("Matrix height", "positive", isdigit, if(const int temp = atoi(pr.second.c_str())) matrix_height = temp; else ERRORMESSAGE("positive"))
			else READPROPERTY(
			    "Matrix width", "positive", isdigit, if(const int temp = atoi(pr.second.c_str())) matrix_width = temp; else ERRORMESSAGE(
			        "positive")) else READPROPERTY("Screen height", "nonnegative", isdigit(ch) || ch == '-' + ch -,
			                                       screen_height = atoi(
			                                           pr.second
			                                               .c_str());) else READPROPERTY("Screen width", "nonnegative", isdigit(ch) || ch == '-' + ch -,
			                                                                             screen_width = atoi(
			                                                                                 pr.second
			                                                                                     .c_str());) else READBOOLEANPROPERTY("Put \'apo\' in screens",
			                                                                                                                          put_apo_in_screens) if(pr.first ==
			                                                                                                                                                 "Saving "
			                                                                                                                                                 "metho"
			                                                                                                                                                 "d") {
				if(pr.second.size() != 1) {
					ERRORMESSAGE("compression_method")
					break;
				}
				const char ch = pr.second[0];
				if(ch == compression_method::lz || ch == compression_method::flz || ch == compression_method::huffman || ch == compression_method::rice8 ||
				   ch == compression_method::riceu8 || ch == compression_method::rle || ch == compression_method::sf)
					saving_method = static_cast<compression_method>(pr.second[0]);
				else {
					ERRORMESSAGE("compression_method")
					break;
				}
			}
		}
		if(was_error) {
			cout << "\n(press enter to continue)";
			cin.get();
		}
	}
#undef READBOOLEANPROPERTY
#undef READPROPERTY
#undef ERRORMESSAGE
	configfile.~ifstream();


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

	WINDOW * main_screen = newwin(screen_height, screen_width, 0, 0);

	scrollok(main_screen, true);
	clearok(main_screen, true);
	for(const string & str : {"Hi,", "I\'m Jedrzej", "From Krakow, Poland.", "Good Mythical Morning!"}) {
		mvwaddstr(main_screen, screen_height / 2, (screen_width - str.size()) / 2, str.c_str());
		wrefresh(main_screen);
		bool broke = false;
		for(unsigned int i = 0; i < screen_height / 2 + 1; ++i) {
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
	game_data * global_data = new game_data;
	load__game_data__from_file(global_data);

	bool shall_keep_going = true;
	while(shall_keep_going)
		switch(const int val = display_mainscreen(main_screen, put_apo_in_screens)) {
			case mainscreen_selection::start:
				play_game(main_screen, global_data->highscore);
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
				display_creditsscreen(main_screen, put_apo_in_screens);
				wclear(main_screen);
				break;
			case mainscreen_selection::options: {
				wclear(main_screen);
				string name = display_optionsscreen(main_screen, string(global_data->name, global_data->length_of_name));
				curs_set(0);
				wclear(main_screen);

				global_data->length_of_name = name.size();
				while(name.size() < GAME_DATA__SERIALIZATION_SIZE)
					name.push_back('\x41');
				memcpy(global_data->name, name.c_str(), GAME_DATA__SERIALIZATION_SIZE);
				save__game_data__to_file(global_data, saving_method);
			} break;
			case mainscreen_selection::highscore:
				wclear(main_screen);
				display_highscorescreen(main_screen, global_data->amount_of_highscores, global_data->highscore);
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
	delete global_data;
	main_screen = nullptr;
	global_data = nullptr;
	endwin();
}


mainscreen_selection display_mainscreen(WINDOW * parent_window, const bool put_apo_in) {
	int maxX, maxY;
	getmaxyx(parent_window, maxY, maxX);
	WINDOW * start_button_window = derwin(parent_window, 3, 7, 0, (maxX - 7) / 2);
	int tempX, tempY;
	getparyx(start_button_window, tempY, tempX);
	WINDOW *tutorial_button_window = derwin(parent_window, 3, 10, tempY + 3, (maxX - 10) / 2),
	       *quit_button_window = derwin(parent_window, 3, 6, maxY - 3, maxX - 6), *credits_button_window = derwin(parent_window, 3, 9, maxY - 3, 0),
	       *options_button_window = derwin(parent_window, 3, 9, maxY - 3, (maxX - 9) / 2);
	getparyx(options_button_window, tempY, tempX);
	WINDOW *highscore_button_window = derwin(parent_window, 3, 10, tempY - 3, (maxX - 11) / 2),
	       *bigstring_message_window = derwin(parent_window, 8, 51, (maxY - 8) / 2, (maxX - 51) / 2);
	touchwin(parent_window);
	wrefresh(parent_window);

#define FOR_ALL_WINDOWS(func)   \
	func(start_button_window);    \
	func(tutorial_button_window); \
	func(quit_button_window);     \
	func(credits_button_window);  \
	func(options_button_window);  \
	func(highscore_button_window);
#define FOR_ALL_WINDOWS_ARG(func, ...)       \
	func(start_button_window, __VA_ARGS__);    \
	func(tutorial_button_window, __VA_ARGS__); \
	func(quit_button_window, __VA_ARGS__);     \
	func(credits_button_window, __VA_ARGS__);  \
	func(options_button_window, __VA_ARGS__);  \
	func(highscore_button_window, __VA_ARGS__);

	FOR_ALL_WINDOWS_ARG(wborder, ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE, ACS_ULCORNER, ACS_URCORNER, ACS_LLCORNER, ACS_LRCORNER)
	mvwaddstr(start_button_window, 1, 1, "Start");
	mvwaddstr(tutorial_button_window, 1, 1, "Tutorial");
	mvwaddstr(quit_button_window, 1, 1, "Quit");
	mvwaddstr(credits_button_window, 1, 1, "Credits");
	mvwaddstr(options_button_window, 1, 1, "Options");
	mvwaddstr(highscore_button_window, 1, 1, "Higscore");
	FOR_ALL_WINDOWS_ARG(mvwchgat, 1, 1, 1, A_BOLD, 0, nullptr);
	FOR_ALL_WINDOWS(wrefresh);

	if(put_apo_in) {
		mvwaddstr(bigstring_message_window, 0, 0, R"(  __    __        __     __   ___________          )");
		mvwaddstr(bigstring_message_window, 1, 0, R"(//  \\//  \\/  \//  \\ //  \\|__   _____ \    _    )");
		mvwaddstr(bigstring_message_window, 2, 0, R"(||    ||  |||  |||  || ||  ||   \ /     \| __| |__ )");
		mvwaddstr(bigstring_message_window, 3, 0, R"(\\__  \\__//\_ |\\__// \\__//   | |       |__   __|)");
		mvwaddstr(bigstring_message_window, 4, 0, R"(    \\  //\\   |/\\  //@@\\/    | |          | |   )");
		mvwaddstr(bigstring_message_window, 5, 0, R"(    || //  \\ /|  \\ ||  |||    | |          | |   )");
		mvwaddstr(bigstring_message_window, 6, 0, R"(\\__////    \|/    \\\\__//\_   |_|          \_\   )");
		mvwaddstr(bigstring_message_window, 4, 23, "\304\304");
	} else {
		mvwaddstr(bigstring_message_window, 0, 0, R"(  __                          ___________          )");
		mvwaddstr(bigstring_message_window, 1, 0, R"(//  \\                       |__   _____ \    _    )");
		mvwaddstr(bigstring_message_window, 2, 0, R"(||                              \ /     \| __| |__ )");
		mvwaddstr(bigstring_message_window, 3, 0, R"(\\__                   __       | |       |__   __|)");
		mvwaddstr(bigstring_message_window, 4, 0, R"(    \\  //\\   //\\  //  \\/    | |          | |   )");
		mvwaddstr(bigstring_message_window, 5, 0, R"(    || //  \\ //  \\ ||  |||    | |          | |   )");
		mvwaddstr(bigstring_message_window, 6, 0, R"(\\__////    \|/    \\\\__//\_   |_|          \_\   )");
	}
	wrefresh(bigstring_message_window);


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

				wmouse_position(start_button_window, mouseX, mouseY);
				if(*mouseX != -1 && *mouseY != -1) {
					toret = mainscreen_selection::start;
					mvwprintw(parent_window, toret, 0, "%d %d\t", *mouseX, *mouseY);
				} else {
					wmouse_position(tutorial_button_window, mouseX, mouseY);
					if(*mouseX != -1 && *mouseY != -1) {
						toret = mainscreen_selection::tutorial;
						mvwprintw(parent_window, toret, 0, "%d %d\t", *mouseX, *mouseY);
					} else {
						wmouse_position(quit_button_window, mouseX, mouseY);
						if(*mouseX != -1 && *mouseY != -1) {
							toret = mainscreen_selection::quit;
							mvwprintw(parent_window, toret, 0, "%d %d\t", *mouseX, *mouseY);
						} else {
							wmouse_position(credits_button_window, mouseX, mouseY);
							if(*mouseX != -1 && *mouseY != -1) {
								toret = mainscreen_selection::credits;
								mvwprintw(parent_window, toret, 0, "%d %d\t", *mouseX, *mouseY);
							} else {
								wmouse_position(options_button_window, mouseX, mouseY);
								if(*mouseX != -1 && *mouseY != -1) {
									toret = mainscreen_selection::options;
									mvwprintw(parent_window, toret, 0, "%d %d\t", *mouseX, *mouseY);
								} else {
									wmouse_position(highscore_button_window, mouseX, mouseY);
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


	FOR_ALL_WINDOWS(delwin);
	delwin(bigstring_message_window);
	start_button_window      = nullptr;
	tutorial_button_window   = nullptr;
	quit_button_window       = nullptr;
	credits_button_window    = nullptr;
	options_button_window    = nullptr;
	highscore_button_window  = nullptr;
	bigstring_message_window = nullptr;

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
	       *name_editbox_window = derwin(parent_window, 3, GAME_DATA__NAME_MAX + 2, 0, (maxX - (GAME_DATA__NAME_MAX + 2)) / 2);
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
			} else if(static_cast<int>(newname.size()) < GAME_DATA__NAME_MAX) {
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

void display_highscorescreen(WINDOW * parent_window, uint8_t amount_of_highscores, const high_data * const highscores) {
	static const unsigned int maximal_score_width = to_string(numeric_limits<uint32_t>::max() / 4).size();
	static const unsigned int maximal_whole_width = GAME_DATA__NAME_MAX + 1 + maximal_score_width + 1 + to_string(numeric_limits<uint16_t>::max() / 4).size();

	int maxX, maxY;
	getmaxyx(parent_window, maxY, maxX);
	WINDOW *menu_button_window         = derwin(parent_window, 3, 6, maxY - 3, maxX - 6),
	       *description_message_window = derwin(parent_window, 2, maximal_whole_width, 0, (maxX - maximal_whole_width) / 2),
	       **highscores_messages_window = new WINDOW *[amount_of_highscores],
	       *none_message_window = amount_of_highscores ? nullptr : derwin(parent_window, 2, 23, maxY - 2, (maxX - 23) / 2);
	for(uint8_t i = 0; i < amount_of_highscores; ++i)
		highscores_messages_window[i] = derwin(parent_window, 1, maximal_whole_width, i + 1, (maxX - maximal_whole_width) / 2), touchwin(parent_window);
	wrefresh(parent_window);

	wborder(menu_button_window, ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE, ACS_ULCORNER, ACS_URCORNER, ACS_LLCORNER, ACS_LRCORNER);
	mvwaddstr(menu_button_window, 1, 1, "Menu");
	mvwchgat(menu_button_window, 1, 1, 1, A_BOLD, 0, nullptr);
	wrefresh(description_message_window);
	mvwaddstr(description_message_window, 0, 0, "Name");
	for(unsigned int i = 4; i < GAME_DATA__NAME_MAX; ++i)
		waddch(description_message_window, ' ');
	waddstr(description_message_window, "|Score");
	for(unsigned int i = 5; i < maximal_score_width; ++i)
		waddch(description_message_window, ' ');
	waddstr(description_message_window, "|Level");
	wrefresh(description_message_window);

	for(uint8_t i = 0; i < amount_of_highscores; ++i) {
		wmove(highscores_messages_window[i], 0, 0);
		wprintw(highscores_messages_window[i], "%.*s", static_cast<int>(highscores[i].length_of_name), highscores[i].name);
		for(unsigned int j = highscores[i].length_of_name; j < GAME_DATA__NAME_MAX; ++j)
			waddch(highscores_messages_window[i], ' ');
		wprintw(highscores_messages_window[i], "|%u", highscores[i].score);
		for(unsigned int j = to_string(highscores[i].score).size(); j < maximal_score_width; ++j)
			waddch(highscores_messages_window[i], ' ');
		wprintw(highscores_messages_window[i], "|%hu", highscores[i].level);
		wrefresh(highscores_messages_window[i]);
	}
	if(!amount_of_highscores) {
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
	if(!amount_of_highscores)
		nodelay(none_message_window, true);
	for(uint8_t i = 0; i < amount_of_highscores; ++i)
		nodelay(highscores_messages_window[i], true);
	mousemask(0, nullptr);

	delwin(menu_button_window);
	delwin(description_message_window);
	for(uint8_t i = 0; i < amount_of_highscores; ++i) {
		delwin(highscores_messages_window[i]);
		highscores_messages_window[i] = nullptr;
	}
	delete[] highscores_messages_window;
	if(!amount_of_highscores) {
		delwin(none_message_window);
		none_message_window = nullptr;
	}
	highscores_messages_window = nullptr;
	description_message_window = nullptr;
	menu_button_window         = nullptr;
}

void play_game(WINDOW * parent_window, high_data * const highscores) {}


// Broken g++ workaround.
template <class T>
constexpr inline string to_string(T val) {
	return static_cast<ostringstream &>(ostringstream() << val).str();
}

void crash_report() {
	time_t time_now(time(nullptr));
	char *time_str = new char[18], *filename;

	const int written = strftime(time_str, 18, "%d.%m.%y %I:%M:%S", localtime(&time_now));

	freopen(filename = const_cast<char *>(("crash." + to_string(clock()) + ".log").c_str()), "w", stderr);
	fprintf(stderr, "Crash report from %s.\nPlease contact the below listed e-mail or poke me on https://github.com/nabijaczleweli.\n\n",
	        written ? time_str : "<<time unknown>>");
	fflush(stderr);

	delete[] time_str;
	time_str = nullptr;
	time_now = -1;
}

unsigned long int filesize(const char * const filename) {
	ifstream file(filename, ios::binary);
	streampos fsize = file.tellg();
	file.seekg(0, ios::end);
	return file.tellg() - fsize;
}


game_data * load__game_data__from_file(game_data * const output_gd, const string & filename) {
	compression_method theremethod;
	const unsigned long int length = filesize(filename.c_str()) - 1;
	unsigned char * inbuffer;
	try {
		inbuffer = new unsigned char[length];
	} catch(const bad_alloc &) {
		return output_gd;
	}

	ifstream(filename, ios::binary).get(reinterpret_cast<char &>(theremethod)).read(static_cast<char *>(static_cast<void *>(inbuffer)), length);
	theremethod = static_cast<compression_method>(theremethod ^ '\xaa');
	switch(theremethod) {
		case compression_method::rle:
			RLE_Uncompress(inbuffer, static_cast<unsigned char *>(static_cast<void *>(output_gd)), length);
			break;
		case compression_method::sf:
			SF_Uncompress(inbuffer, static_cast<unsigned char *>(static_cast<void *>(output_gd)), length, GAME_DATA__SERIALIZATION_SIZE);
			break;
		case compression_method::huffman:
			Huffman_Uncompress(inbuffer, static_cast<unsigned char *>(static_cast<void *>(output_gd)), length, GAME_DATA__SERIALIZATION_SIZE);
			break;
		case compression_method::rice8:
			Rice_Uncompress(inbuffer, static_cast<unsigned char *>(static_cast<void *>(output_gd)), length, GAME_DATA__SERIALIZATION_SIZE, RICE_FMT_INT8);
			break;
		case compression_method::riceu8:
			Rice_Uncompress(inbuffer, static_cast<unsigned char *>(static_cast<void *>(output_gd)), length, GAME_DATA__SERIALIZATION_SIZE, RICE_FMT_UINT8);
			break;
		case compression_method::flz:
		case compression_method::lz:
			LZ_Uncompress(inbuffer, static_cast<unsigned char *>(static_cast<void *>(output_gd)), length);
			break;
	}

	memset(inbuffer, length, '\0');
	delete[] inbuffer;
	inbuffer = nullptr;

	return output_gd;
}

void save__game_data__to_file(const game_data * const input_gd, compression_method method, const string & filename) {
	unsigned int outbuflength;
	switch(method) {
		case compression_method::rle:
			outbuflength = static_cast<unsigned int>(ceil(static_cast<double>(GAME_DATA__SERIALIZATION_SIZE) * (257.0 / 256.0))) + 1;
			break;
		case compression_method::sf:
			outbuflength = static_cast<unsigned int>(ceil(static_cast<double>(GAME_DATA__SERIALIZATION_SIZE) * (101.0 / 100.0))) + 384;
			break;
		case compression_method::huffman:
			outbuflength = static_cast<unsigned int>(ceil(static_cast<double>(GAME_DATA__SERIALIZATION_SIZE) * (101.0 / 100.0))) + 320;
			break;
		case compression_method::rice8:
		case compression_method::riceu8:
			outbuflength = GAME_DATA__SERIALIZATION_SIZE + 320;
			break;
		case compression_method::lz:
		case compression_method::flz:
			outbuflength = static_cast<unsigned int>(ceil(static_cast<double>(GAME_DATA__SERIALIZATION_SIZE) * (257.0 / 256.0))) + 1;
			break;
		default:
			crash_report();
			throw simplesmart_exception("Wrong `method` enum value passed to `save__game_data__to_file` (it being: \'" +
			                            to_string(static_cast<compression_method>(method)) + "\').");
	}
	unsigned char * outbuffer = new unsigned char[outbuflength];

	int destLen;
	switch(method) {
		case compression_method::rle:
			destLen = RLE_Compress(static_cast<unsigned char *>(static_cast<void *>(const_cast<game_data *>(input_gd))), outbuffer, GAME_DATA__SERIALIZATION_SIZE);
			break;
		case compression_method::sf:
			destLen = SF_Compress(static_cast<unsigned char *>(static_cast<void *>(const_cast<game_data *>(input_gd))), outbuffer, GAME_DATA__SERIALIZATION_SIZE);
			break;
		case compression_method::huffman:
			destLen =
			    Huffman_Compress(static_cast<unsigned char *>(static_cast<void *>(const_cast<game_data *>(input_gd))), outbuffer, GAME_DATA__SERIALIZATION_SIZE);
			break;
		case compression_method::rice8:
			destLen = Rice_Compress(static_cast<unsigned char *>(static_cast<void *>(const_cast<game_data *>(input_gd))), outbuffer, GAME_DATA__SERIALIZATION_SIZE,
			                        RICE_FMT_INT8);
			break;
		case compression_method::riceu8:
			destLen = Rice_Compress(static_cast<unsigned char *>(static_cast<void *>(const_cast<game_data *>(input_gd))), outbuffer, GAME_DATA__SERIALIZATION_SIZE,
			                        RICE_FMT_UINT8);
			break;
		case compression_method::flz: {
			unsigned int * work_memory = new unsigned int[outbuflength + 65536];
			destLen = LZ_CompressFast(static_cast<unsigned char *>(static_cast<void *>(const_cast<game_data *>(input_gd))), outbuffer, GAME_DATA__SERIALIZATION_SIZE,
			                          work_memory);
			memset(work_memory, outbuflength + 65536, 0);
			delete[] work_memory;
			work_memory = nullptr;
		} break;
		case compression_method::lz:
			destLen = LZ_Compress(static_cast<unsigned char *>(static_cast<void *>(const_cast<game_data *>(input_gd))), outbuffer, GAME_DATA__SERIALIZATION_SIZE);
			break;
		default:
			memset(outbuffer, outbuflength, '\0');
			delete[] outbuffer;
			outbuffer = nullptr;

			crash_report();
			throw simplesmart_exception("Wrong `method` enum value passed to `save__game_data__to_file` (it being: \'" +
			                            to_string(static_cast<compression_method>(method)) + "\').");
	}
	ofstream(filename, ios::binary).put(method ^ '\xaa').write(static_cast<char *>(static_cast<void *>(outbuffer)), destLen);

	memset(outbuffer, outbuflength, '\0');
	delete[] outbuffer;
	outbuffer = nullptr;
}
