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


#include "exceptions.hpp"

#include <ctime>


using namespace std;


simplesmart_exception::simplesmart_exception(const char * const msg)
      : message("Error in apoSimpleSmart: "s + msg + ".\n\t\tPlease contact nabijaczleweli@gmail.com for further support.") {}

simplesmart_exception::simplesmart_exception(const string & msg) : simplesmart_exception(msg.c_str()) {}
simplesmart_exception::simplesmart_exception(const simplesmart_exception & exc) : message(exc.message) {}
simplesmart_exception::simplesmart_exception(simplesmart_exception && exc) : message(move(exc.message)) {}

const char * simplesmart_exception::what() const noexcept {
	return message.c_str();
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
