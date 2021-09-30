CWD = $(notdir $(shell pwd))
CWD_A = $(shell pwd)


# source files.
SRC := $(wildcard ./src/*.cpp)


$(info $(SRC))


OBJ = $(SRC:.cpp=.o)
 

OUT = build/debug/lib$(CWD).a

 
# include directories
INCLUDES =  -I/usr/include/ \
            -I/usr/local/include/ \
            -I./inc/ \
            -I$(CWD_A)/../FPGA_shim/inc/ \
            -I$(CWD_A)/../syscNetProto/inc/ \
            -I$(CWD_A)/../network/inc/

 
# C++ compiler flags (-g -O0 -O1 -O2 -O3 -Wall -std=c++14 -std=c++11 -fPIC -fexceptions)
CCFLAGS = -Wall -std=c++1y -g -fPIC -fexceptions


# compiler
CCC = g++


# library paths
LIBS = -L/usr/lib/ -L/usr/local/lib/ -lm


.SUFFIXES: .cpp


default: $(OUT)


.cpp.o:
	$(CCC) $(INCLUDES) $(CCFLAGS) -c $< -o $@


$(OUT): $(OBJ)
	ar rcs $(OUT) $(OBJ)


clean:
	rm -f $(OBJ) $(OUT) Makefile.bak
