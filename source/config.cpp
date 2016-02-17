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


#define GAME_DATA__NAME_MAX 33
#define GAME_DATA__HIGHSCORE_MAX_AMOUNT 10

#define HIGH_DATA__SERIALIZATION_SIZE (sizeof(uint8_t) + GAME_DATA__NAME_MAX + sizeof(uint32_t) + sizeof(uint16_t))
#define GAME_DATA__SERIALIZATION_SIZE \
	(sizeof(uint8_t) + GAME_DATA__NAME_MAX + sizeof(uint8_t) + GAME_DATA__HIGHSCORE_MAX_AMOUNT * HIGH_DATA__SERIALIZATION_SIZE)


using namespace std;
using namespace std::experimental;


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
				return {nullopt, 0};
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

	return {make_optional(ass_config{
		matrix_width,
		matrix_height,
		screen_width,
		screen_height,
		put_apo_in_screens,
		saving_method
	}), 0};
}
