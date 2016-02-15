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


LDAR = $(foreach subproj,$(SUBPROJS),-L./$(subproj) -l$(subproj)) -lbcl -lpdcurses -larmadillo


.PHONY : clean all $(SUBPROJS)


all : aSS$(OBJ) $(foreach subproj,$(SUBPROJS),$(subproj)$(SUBPROJMARKER))
	$(CPP) $(CPPAR) $(SYSLDAR) $(LDAR) *$(OBJ) -oapoSimpleSmart$(EXE)
	$(STRIP) $(STRIPAR) apoSimpleSmart$(EXE) -oapoSimpleSmart$(EXE)

clean :
	$(foreach subproj,$(SUBPROJS),$(MAKE) -C $(subproj) clean &&) $(nop)
	rm -rf *$(OBJ) *$(EXE)


%$(OBJ) : %.cpp
	$(CPP) $(CPPAR) $(foreach subproj,$(SUBPROJS),-I./$(subproj)) -c -o$@ $^

%$(SUBPROJMARKER) : %
	$(MAKE) -C$^ all