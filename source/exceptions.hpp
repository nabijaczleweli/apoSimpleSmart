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


#pragma once


#include <stdexcept>
#include <string>


class simplesmart_exception : public std::exception {
private:
	std::string message;

public:
	simplesmart_exception(const char * const msg)
	      : message("Error in apoSimpleSmart: " + msg + ".\n\t\tPlease contact nabijaczleweli@gmail.com for further support.") {}

	simplesmart_exception(const std::string & msg) : simplesmart_exception(msg.c_str()) {}
	simplesmart_exception(const simplesmart_exception & exc) : message(exc.message) {}
	simplesmart_exception(simplesmart_exception && exc) : message(std::move(exc.message)) {}

	const char * what() const noexcept {
		return message.c_str();
	}
};
