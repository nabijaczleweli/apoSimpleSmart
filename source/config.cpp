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

#include "game_data.hpp"


using namespace std;
using namespace std::experimental;


static void print_error(const char * progname, const string & non_what, const string & config_filename, const pair<const string, string> & pr);
static bool parse_boolean(const char * progname, const string & config_filename, const pair<const string, string> & pr, bool & error);


pair<optional<ass_config>, int> parse_options(const char * const * argv) {
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


	unsigned int matrix_width = 7, matrix_height = 7, screen_width = 80, screen_height = 25;
	bool put_apo_in_screens = false;
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
		                         << put_apo_in_screens << "\n";
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

		bool was_error = false;
		for(const auto & pr : values) {
			if(pr.first == "Matrix height") {
				bool bad = false;
				for(const char & ch : pr.second)
					if(!isdigit(ch)) {
						print_error(*argv, "positive", configfilename, pr);
						was_error = true;
						bad       = true;
						break;
					}
				if(bad)
					continue;

				if(const int temp = atoi(pr.second.c_str()))
					matrix_height = temp;
				else
					print_error(*argv, "positive", configfilename, pr);
			} else if(pr.first == "Matrix width") {
				bool bad = false;
				for(const char & ch : pr.second)
					if(!isdigit(ch)) {
						print_error(*argv, "positive", configfilename, pr);
						was_error = true;
						bad       = true;
						break;
					}
				if(bad)
					continue;

				if(const int temp = atoi(pr.second.c_str()))
					matrix_width = temp;
				else
					print_error(*argv, "positive", configfilename, pr);
			} else if(pr.first == "Screen height") {
				bool bad = false;
				for(const char & ch : pr.second)
					if(!isdigit(ch) || ch == '-') {
						print_error(*argv, "nonnegative", configfilename, pr);
						was_error = true;
						bad       = true;
						break;
					}
				if(bad)
					continue;

				screen_height = atoi(pr.second.c_str());
			} else if(pr.first == "Screen width") {
				bool bad = false;
				for(const char & ch : pr.second)
					if(!isdigit(ch) || ch == '-') {
						print_error(*argv, "nonnegative", configfilename, pr);
						was_error = true;
						bad       = true;
						break;
					}
				if(bad)
					continue;

				screen_width = atoi(pr.second.c_str());
			} else if(pr.first == "Put \'apo\' in screens")
				put_apo_in_screens = parse_boolean(*argv, configfilename, pr, was_error);
		}
		if(was_error) {
			cout << "\n(press enter to continue)";
			cin.get();
		}
	}

	return {make_optional(ass_config{matrix_width, matrix_height, screen_width, screen_height, put_apo_in_screens}), 0};
}


static void print_error(const char * progname, const string & non_what, const string & config_filename, const pair<const string, string> & pr) {
	cerr << progname << ": warning: non-" << non_what << " value in \"" << config_filename << "\" at key \"" << pr.first << "\" for value \"" << pr.second
	     << "\" reverting to previous/default\n";
}

static bool parse_boolean(const char * progname, const string & config_filename, const pair<const string, string> & pr, bool & error) {
	switch(pr.second.size()) {
		case 1:
			switch(pr.second[0]) {
				case '0':
					return false;
					break;
				case '1':
					return true;
					break;
				default:
					print_error(progname, "boolean", config_filename, pr);
					error = true;
			}
			break;
		case 2:
			if(pr.second == "no")
				return false;
			else {
				print_error(progname, "boolean", config_filename, pr);
				error = true;
			}
			break;
		case 3:
			if(pr.second == "yes")
				return true;
			else {
				print_error(progname, "boolean", config_filename, pr);
				error = true;
			}
			break;
		case 4:
			if(pr.second == "true")
				return true;
			else {
				print_error(progname, "boolean", config_filename, pr);
				error = true;
			}
			break;
		case 5:
			if(pr.second == "false")
				return false;
			else {
				print_error(progname, "boolean", config_filename, pr);
				error = true;
			}
			break;
		default:
			print_error(progname, "lengthy-enough (1..5) (actual length: " + to_string(pr.second.size()) + ")", config_filename, pr);
			error = true;
	}


	return false;
}
