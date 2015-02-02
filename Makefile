GCC=g++
CFLAGS= -Wall -g
MAKE=gmake
LIBS= /usr/local/lib
LUA_PATH=./lua-5.1.4/src
LUA_LIB=$(LUA_PATH)/liblua.a

all:luamain lighttable

luamain:lighttable.o ltutil.o lmap.o ltlua.o luamain.cpp
	$(GCC) -o $@ $^ -L/$(LIBS) $(LUA_LIB) -I $(LUA_PATH) -lpthread -g

lighttable:lighttable.o ltutil.o lmap.o main.cpp
	$(GCC) -o $@ $^ -L/$(LIBS) -lpthread -g

%.o:%.cpp
	$(GCC) -c $(CFLAGS) $^ -I $(LUA_PATH) -o $@ 

.PHONY:clean
clean:
	-rm -rf *.o 
	-rm luamain lighttable
	-rm -rf *core*
