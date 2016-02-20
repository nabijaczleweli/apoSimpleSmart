// The MIT License (MIT)

// Copyright (c) 2016 nabijaczleweli

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


#include "config.hpp"

#include <regex>
#include <cstring>
#include <fstream>
#include <iostream>
#include <unordered_map>

#include "cereal/cereal.hpp"
#include "cereal/archives/json.hpp"
#include "tclap/CmdLine.h"

#include "game_data.hpp"


using namespace std;
using namespace std::experimental;
using namespace TCLAP;


template <class Archive>
void serialize(Archive & archive, ass_config & ac) {
	archive(cereal::make_nvp("Matrix width", ac.matrix_width), cereal::make_nvp("Matrix height", ac.matrix_height),
	        cereal::make_nvp("Screen width", ac.screen_width), cereal::make_nvp("Screen height", ac.screen_height),
	        cereal::make_nvp("Put 'apo' in screens", ac.put_apo_in_screens));
}


static pair<optional<string>, int> commandline_options(int argc, const char * const * argv);


pair<optional<ass_config>, int> parse_options(int argc, const char * const * argv) {
	const auto commandline = commandline_options(argc, argv);
	if(!commandline.first)
		return {nullopt, commandline.second};
	const auto configfilename = commandline.first.value();


	ass_config cfg;
	fstream configfile(configfilename, ios::in);
	if(!configfile.is_open()) {
		configfile.open(configfilename, ios::out);
		cereal::JSONOutputArchive archive(configfile);
		archive(cereal::make_nvp("apoSimpleSmart configuration", cfg));
	} else {
		cereal::JSONInputArchive archive(configfile);
		try {
			archive(cfg);
		} catch(cereal::RapidJSONException &) {
		}
	}

	return {make_optional(cfg), 0};
}


static pair<optional<string>, int> commandline_options(int argc, const char * const * argv) {
	try {
		CmdLine command_line("apoSimpleSmart -- Curses clone of APO SimpleSmart Android game", ' ', __DATE__ " " __TIME__);

		ValueArg<string> configfile("c", "configfile", "Use config file FILE; Default: simple_smart.cfg", false, "simple_smart.cfg", "FILE", command_line);
		command_line.parse(argc, argv);

		return {make_optional(regex_replace(configfile.getValue(), regex("\\\\"), "/")), 0};
	} catch(const ArgException &) {
	}
	return {nullopt, 1};
}
