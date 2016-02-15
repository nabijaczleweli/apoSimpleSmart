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

#ifndef EXCEPTIONS_HPP
#define EXCEPTIONS_HPP

#include <stdexcept>
#include <string>

class simplesmart_exception : public std::exception {
	private:
		unsigned int msglen, messagelen;
		char * message;
	public:
		simplesmart_exception(const std::string & msg) : simplesmart_exception(msg.c_str()) {}
		simplesmart_exception(const char * const msg) : msglen(strlen(msg)), messagelen(msglen + 89), message(new char[messagelen + 1]) {
			memcpy(message, "Error in apoSimpleSmart: ", 25);
			memcpy(message + 25, msg, msglen);
			memcpy(message + 25 + msglen, ".\n\t\tPlease contact nabijaczleweli@gmail.com for further support.", 65);
		}
		simplesmart_exception(const simplesmart_exception & exc) : msglen(exc.msglen), messagelen(exc.messagelen), message(new char[messagelen + 1]) {
			memcpy(message, exc.message, messagelen + 1);
		}
		simplesmart_exception(simplesmart_exception && exc) : msglen(exc.msglen), messagelen(exc.messagelen), message(exc.message) {
			exc.msglen = -1;
			exc.messagelen = -1;
			exc.message = 0;
		}
		const char * what() const noexcept(true) {
			return message;
		}
		~simplesmart_exception() {
			delete[] message;
			message = nullptr;
		}
		simplesmart_exception & operator=(const simplesmart_exception & other) {
			delete[] message;

			msglen = other.msglen;
			messagelen = other.messagelen;
			message = new char[messagelen + 1];
			memcpy(message, other.message, messagelen + 1);

			return *this;
		}
};

#endif  // EXCEPTIONS_HPP

