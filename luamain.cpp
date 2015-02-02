#include "ltlua.h"
#include "lighttable.h"

#include <iostream>
using namespace std;

lua_State *L =NULL;

const char *predo_path = "./test.lua";

void init_lua()
{
	if (L != NULL)
		return;

	L = luaL_newstate();
	luaL_openlibs(L);
	
	init_light_table(L);
	
	int error = 0;
	error = luaL_loadfile(L, predo_path);
	if (error != 0) {
		cout<<"sssss"<<endl;
	}

	error = lua_pcall(L, 0,0,0);
	if (error != 0) {
		cout<<"tttt"<<endl;
	}
}

void call_lua()
{
	lua_getglobal(L, "fun");
	if (lua_pcall(L, 0, 0, 0) != 0)
		cout<<"what the hell"<<endl;
}

int main(int argc, char **argv)
{
	init_lua();	

	call_lua();
	lua_close(L);
	return 0;
}
