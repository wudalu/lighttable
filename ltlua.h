#ifndef LTLUA_H
#define LTLUA_H

const int MAX_KEY_SIZE = 100;

extern "C" {
	#include <lua.h>
	#include <lualib.h>
	#include <lauxlib.h>
}

int init_light_table(lua_State *L);

#endif //LTLUA_H
