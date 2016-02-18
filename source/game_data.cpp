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


#include "game_data.hpp"

#include <fstream>

#include "cereal/cereal.hpp"
#include "cereal/types/vector.hpp"
#include "cereal/archives/json.hpp"


using namespace std;


template <class Archive>
void serialize(Archive & archive, high_data & hd) {
	archive(hd.name, hd.score, hd.level);
}

template <class Archive>
void serialize(Archive & archive, game_data & gd) {
	archive(gd.name, gd.highscore);
}


game_data load_game_data_from_file(const std::string & filename) {
	game_data res;

	ifstream ifs(filename);
	cereal::JSONInputArchive archive(ifs);
	archive(res);

	return res;
}

void save_game_data_to_file(const game_data & input_gd, const std::string & filename) {
	vector<string> asdf;
	ofstream ofs(filename);
	cereal::JSONOutputArchive archive(ofs);
	archive(input_gd);
}
