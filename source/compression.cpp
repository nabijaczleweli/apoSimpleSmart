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


#include "compression.hpp"

#include <fstream>
#include <cstring>
#include <cmath>

#include "bcl/src/huffman.h"
#include "bcl/src/lz.h"
#include "bcl/src/rice.h"
#include "bcl/src/rle.h"
#include "bcl/src/shannonfano.h"

#include "exceptions.hpp"


#define GAME_DATA__NAME_MAX 33
#define GAME_DATA__HIGHSCORE_MAX_AMOUNT 10

#define HIGH_DATA__SERIALIZATION_SIZE (sizeof(uint8_t) + GAME_DATA__NAME_MAX + sizeof(uint32_t) + sizeof(uint16_t))
#define GAME_DATA__SERIALIZATION_SIZE \
	(sizeof(uint8_t) + GAME_DATA__NAME_MAX + sizeof(uint8_t) + GAME_DATA__HIGHSCORE_MAX_AMOUNT * HIGH_DATA__SERIALIZATION_SIZE)


using namespace std;


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

	memset(inbuffer, '\0', length);
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
			memset(work_memory, 0, outbuflength + 65536);
			delete[] work_memory;
			work_memory = nullptr;
		} break;
		case compression_method::lz:
			destLen = LZ_Compress(static_cast<unsigned char *>(static_cast<void *>(const_cast<game_data *>(input_gd))), outbuffer, GAME_DATA__SERIALIZATION_SIZE);
			break;
		default:
			memset(outbuffer, '\0', outbuflength);
			delete[] outbuffer;
			outbuffer = nullptr;

			crash_report();
			throw simplesmart_exception("Wrong `method` enum value passed to `save__game_data__to_file` (it being: \'" +
			                            to_string(static_cast<compression_method>(method)) + "\').");
	}
	ofstream(filename, ios::binary).put(method ^ '\xaa').write(static_cast<char *>(static_cast<void *>(outbuffer)), destLen);

	memset(outbuffer, '\0', outbuflength);
	delete[] outbuffer;
	outbuffer = nullptr;
}


ostream & operator<<(ostream & strm, compression_method method) {
	return strm << static_cast<char>(method);
}
