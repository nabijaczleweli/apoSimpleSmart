# The MIT License (MIT)

# Copyright (c) 2014 nabijaczleweli

# Permission is hereby granted, free of charge, to any person obtaining a copy of
# this software and associated documentation files (the "Software"), to deal in
# the Software without restriction, including without limitation the rights to
# use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
# the Software, and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


include configMakefile


OBJECTS := $(patsubst source/%.cpp,out/%.o,$(wildcard source/**.cpp))


.PHONY : all clean exe rust


all : rust exe

clean :
	rm -rf out

exe : $(OBJECTS)
	$(CXX) $(CXXAR) $^ -oout/apoSimpleSmart $(LDAR) $(shell cat out/dependencies/librust_helpers.deps)

rust : out/dependencies/librust_helpers.a


out/dependencies/librust_helpers.a : $(wildcard dependencies/rust_helpers/**.rs)
	@mkdir -p $(dir $@)
	cd $(dir $@) && $(RS) $(RSAR) rust_helpers 2>&1 $(foreach s,$^,$(realpath $(s))) | grep "note: library:" | perl -pe "s/note: library: (.+)/-l\1/" > librust_helpers.deps


out/%$(OBJ) : source/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXAR) -Idependencies/cereal/include -c -o$@ $^
