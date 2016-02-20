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

#include <cstring>
#include <fstream>
#include <iostream>
#include <unordered_map>

#include "cereal/cereal.hpp"
#include "cereal/archives/json.hpp"

#include "game_data.hpp"


using namespace std;
using namespace std::experimental;


template <class Archive>
void serialize(Archive & archive, ass_config & ac) {
	archive(cereal::make_nvp("Matrix width", ac.matrix_width), cereal::make_nvp("Matrix height", ac.matrix_height),
	        cereal::make_nvp("Screen width", ac.screen_width), cereal::make_nvp("Screen height", ac.screen_height),
	        cereal::make_nvp("Put 'apo' in screens", ac.put_apo_in_screens));
}


static pair<optional<string>, int> commandline_options(const char * const * argv);


pair<optional<ass_config>, int> parse_options(const char * const * argv) {
	const auto commandline = commandline_options(argv);
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
		} catch(cereal::RapidJSONException &) {}
	}

	return {make_optional(cfg), 0};
}


static pair<optional<string>, int> commandline_options(const char * const * argv) {
	string configfilename = "simple_smart.cfg";

	for(unsigned int idx = 1; argv[idx]; ++idx) {
		const char * const arg    = argv[idx];
		const unsigned int arglen = strlen(arg);
		if(*arg == '-' && arg[1]) {
			if(arglen >= 13 && !memcmp(arg, "--configfile=", 13)) {
				if(arglen == 13) {
					cerr << *argv << ": error: missing filename after \'--configfile=\'\n";
					return {nullopt, 1};
				}
				configfilename = arg + 13;
			} else if(arglen >= 2 && !memcmp(arg, "-c", 2)) {
				if(arglen == 2) {
					if(!argv[idx + 1]) {
						cerr << *argv << ": error: missing filename after \'-c\'\n";
						return {nullopt, 1};
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
					                              "  --configfile=<file>       Use config file file; Default: simple_smart.cfg\n"
					                              "  -c <file>                 Use config file file; Default: simple_smart.cfg\n"
					                              "\n"
					                              "For bug reporting instructions, please see:\n"
					                              "<https://github.com/nabijaczleweli/apoSimpleSmart>.\n";
				}
				return {nullopt, 0};
			}
		}
	}

	return {make_optional(configfilename), 0};
}
