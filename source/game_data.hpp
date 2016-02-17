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


#pragma once


#include <cstdint>


#define GAME_DATA__NAME_MAX 33
#define GAME_DATA__HIGHSCORE_MAX_AMOUNT 10

#define HIGH_DATA__SERIALIZATION_SIZE (sizeof(uint8_t) + GAME_DATA__NAME_MAX + sizeof(uint32_t) + sizeof(uint16_t))
#define GAME_DATA__SERIALIZATION_SIZE \
	(sizeof(uint8_t) + GAME_DATA__NAME_MAX + sizeof(uint8_t) + GAME_DATA__HIGHSCORE_MAX_AMOUNT * HIGH_DATA__SERIALIZATION_SIZE)


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

	game_data();
};
