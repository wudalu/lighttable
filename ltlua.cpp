#include "ltlua.h"
#include "lighttable.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <ctime>
#include <sys/time.h>

//forward declearation
static int _next_array(lua_State *L, table *t, int index);

static void _get_value(lua_State *L, int ttype, union table_value *tv, table *t)
{
	switch (ttype) {
		case LT_NIL:
			lua_pushnil(L);
			break;

		case LT_NUMBER:
			lua_pushnumber(L, tv->n);
			break;

		case LT_BOOLEAN:
			lua_pushboolean(L, tv->b);
			break;

		case LT_UD:
			lua_pushlightuserdata(L, (void *)(uintptr_t)tv->ud);
			break;

		case LT_STRING:
			t->value_string(tv, (table_setstring_func)lua_pushlstring, L);
			break;

		default:
			luaL_error(L, "Invalid type %d", ttype);
	}
}

static int _create(lua_State *L)
{
	table *t = new table;
	lua_pushlightuserdata(L, t);
	luaL_getmetatable(L, "ltmeta");
	lua_setmetatable(L, -2);

	return 1;
}

static int _delete(lua_State *L)
{
	table *t = (table *)lua_touserdata(L, 1);
	delete t;
	t = NULL;

	return 0;
}

static int _serialize(lua_State *L)
{
	int type = lua_type(L, 1);
	assert(type == LUA_TLIGHTUSERDATA);

	table *t = (table *)lua_touserdata(L,1);
	t->serialize();
	return 0;
}

static int _size(lua_State *L)
{
	table *t = (table *)lua_touserdata(L, 1);

	size_t cap = t->size();
	struct table_key *tv = (struct table_key *)malloc(cap * sizeof(*tv));
	size_t size = t->keys(tv, cap);

	assert(size <= cap);
	lua_pushnumber(L, size);

	free(tv);
	tv = NULL;

	return 1;
}

static int _ipairsaux(lua_State *L)
{
	int i = luaL_checkint(L, 2);
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);

	table *t = (table *)lua_touserdata(L, 1);
	union table_value tv;
	i++;
	lua_pushinteger(L, i);

	int ttype = t->get_type(NULL, i, &tv);
	_get_value(L, ttype, &tv, t);

	return (lua_isnil(L, -1)) ? 0 : 2;
}

/*这个地方非常有趣，值得记录一下
 *_ipairs返回3个参数
 *在lua中
 *for namelist in iterator do
 *	block
 *end
 *相当于
 *while true do
 *	local namelist = iterator()
 *	if nil == first(namelist) then break end
 *	block
 *end
 *对应到3个返回值，按照压栈的顺序，分别是iterator, table, index 
 *也就是，三个参数返回后，lua端的for接管了这个循环的过程
 *for循环做的事情就是，调用iterator,传入index,对table中index依次返回
 *循环停止的条件是iterator返回的第一个值为nil
 */
static int _ipairs(lua_State *L)
{
	lua_pushcfunction(L, _ipairsaux);  //iterator
	lua_pushvalue(L, 1);               //table,our table
	lua_pushinteger(L, 0);             //index

	return 3;
}

static void _push_upvalues(lua_State *L, struct table_key *keys, int size)
{
	if (size == 0) {
		lua_pushinteger(L, 0);
		lua_pushinteger(L, 0);
		lua_pushinteger(L, 0);
		lua_pushnil(L);
		return ;
	}

	int array_size = 0;
	int map_size = 0;
	map_size = size;

	lua_pushinteger(L, array_size);
	lua_pushinteger(L, map_size);
	lua_pushinteger(L, 0);

	if (map_size > 0) {
		void *ud = lua_newuserdata(L, map_size * sizeof(struct table_key));
		memcpy(ud, keys, map_size * sizeof(struct table_key));
	}
	else {
		lua_pushnil(L);
	}
}

static int _next_map(lua_State *L, table *t)
{
	int map_size = lua_tointeger(L, lua_upvalueindex(2));
	if (map_size == 0)
		return 0;

	int position = lua_tointeger(L, lua_upvalueindex(3));
	if (position >= map_size) {
		lua_pushinteger(L, 0);
		lua_replace(L, lua_upvalueindex(3));
		//map优先
		//return _next_array(L, t, 0);
		//array优先
		return 0;
	}
	lua_pushinteger(L, position +1);
	lua_replace(L, lua_upvalueindex(3));

	union table_value tv;
	int ttype;
	struct table_key *keys = (struct table_key *)lua_touserdata(L, lua_upvalueindex(4));
	if (keys[position].key != NULL) {
		lua_pushlstring(L, keys[position].key, keys[position].sz_idx);
		ttype = t->get_type(keys[position].key, keys[position].sz_idx, &tv);
	}
	else {
		lua_pushnumber(L, keys[position].sz_idx);
		ttype = t->get_type(NULL, keys[position].sz_idx, &tv);
	}


	_get_value(L, ttype, &tv, t);

	return 2;
}

static int _next_array(lua_State *L, table *t, int pre)
{
	int array_size = lua_tointeger(L, lua_upvalueindex(1));
	if (pre >= array_size) {
		//array优先
		return _next_map(L, t);
		//map优先
		//return 0;
	}

	int ttype;
	union table_value tv;
	for (int i=pre; i<array_size; i++) {
		ttype = t->get_type(NULL, i, &tv);
		if (ttype == LT_NIL) 
			continue;
		lua_pushinteger(L, i+1);
		_get_value(L, ttype, &tv, t);
		return 2;
	}
	
	return _next_map(L, t);
}

static int _pairsaux(lua_State *L)
{
	table *t = (table *)lua_touserdata(L, 1);
	int type = lua_type(L, 2);
	switch (type) {
		case LUA_TNIL:
			//array优先
			//return _next_array(L, t, 0);
			//map优先
			return _next_map(L, t);
			
		case LUA_TNUMBER:
			//return _next_array(L, t, lua_tointeger(L, 2));
			return _next_map(L, t);

		case LUA_TSTRING:
			return _next_map(L, t);

		default:
			return luaL_error(L, "Invalid next key");
	}
}

static int _pairs(lua_State *L)
{
	table *t = (table *)lua_touserdata(L, 1);

	size_t cap = t->size();
	struct table_key *keys = (struct table_key *)malloc(cap * sizeof(*keys));
	int size = t->keys(keys, cap);
	
	//push upvalues
	_push_upvalues(L, keys, size);
	//push cclosure
	lua_pushcclosure(L, _pairsaux, 4);

	free(keys);
	keys = NULL;

	//push t
	lua_pushvalue(L, 1);

	return 2;
}

static int _get(lua_State *L)
{
	table *t = (table *)lua_touserdata(L, 1);
	union table_value tv;
	int ttype;
	int idx;

	int type = lua_type(L, 2);
	switch (type) {
		case LUA_TNUMBER:
			idx = lua_tointeger(L, 2);
			if (idx <= 0) {
				return luaL_error(L, "Unsupported index %d", idx);
			}

			ttype = t->get_type(NULL, idx-1, &tv);
			break;

		case LUA_TSTRING:
			size_t sz;
			const char *key = lua_tolstring(L, 2, &sz);
			ttype = t->get_type(key, sz, &tv);
	}

	_get_value(L, ttype, &tv, t);

	return 1;
}

static const char *_get_key(lua_State *L, int key_idx, size_t *sz_idx, table *t)
{
	int type = lua_type(L, key_idx);
	const char *key;
	size_t sz;
	switch (type) {
		case LUA_TNUMBER:
			sz = lua_tointeger(L, key_idx);	
			if (sz < 0) 
				luaL_error(L, "Unsuopported index %d", sz);

			key = NULL;
			*sz_idx = sz;
			break;

		case LUA_TSTRING:
			key = lua_tolstring(L, key_idx, sz_idx); 
			break;

		default:
			luaL_error(L, "Unsupported key type %s", lua_typename(L, type));
	}

	return key;
}

static void _error(lua_State *L, const char *key, size_t sz, int type)
{
	if (key == NULL) {
		luaL_error(L, "Can't set %d with type %s", (int)sz, lua_typename(L, type));
		return ;
	}

	luaL_error(L, "Can't set %s with type %s", key, lua_typename(L, type));
}

static void _set_value(lua_State *L, table *t, const char *key, size_t sz, int idx)
{
	int type = lua_type(L, idx);
	size_t len;
	const char *str;
	int ret;
	switch (type) {
		case LUA_TNUMBER:
			ret = t->set_number(key, sz, lua_tonumber(L, idx));
			break;

		case LUA_TBOOLEAN:
			ret = t->set_boolean(key, sz, lua_toboolean(L, idx));
			break;

		case LUA_TSTRING:
			str = lua_tolstring(L, idx, &len);
			ret = t->set_string(key, sz, str, len);
			break;

		case LUA_TLIGHTUSERDATA:
			ret = t->set_ud(key, sz, (uint64_t)(uintptr_t)lua_touserdata(L, idx));
			break;

		default:
			luaL_error(L, "Unsupported value type %s", lua_typename(L, type));
	}

	if (ret != 0) {
		_error(L, key, sz, type);
	}
}

static int _set(lua_State *L)
{
	table *t = (table *)lua_touserdata(L, 1);
	size_t sz;
	const char *key = _get_key(L, 2, &sz, t);
	_set_value(L, t, key, sz, 3);

	return 0;
}

table * do_convert(lua_State *L)
{	
	table *t = new table();

	lua_pushnil(L);
	while (lua_next(L, -2)) {
		lua_pushvalue(L, -2);
		int type = lua_type(L,-1);
		int value_type = lua_type(L, -2);

		size_t sz;
		const char *key = _get_key(L, -1, &sz, t);
		if (value_type == LUA_TTABLE) {
			lua_pushvalue(L, -2);
			table *sub = do_convert(L);
			//sub->serialize();
			t->set_ud(key, sz, (uint64_t)(uintptr_t)sub);
			lua_pop(L,3);
		}
		else {
			_set_value(L, t, key, sz, -2);
			lua_pop(L,2);
		}
	}

	return t;
}

static int _convert(lua_State *L)
{
	struct timeval stv;
	gettimeofday(&stv, NULL);
	table *t = do_convert(L);

	struct timeval etv;
	gettimeofday(&etv, NULL);
	//printf("convert time from cpp is %d\n", (etv.tv_sec * 1000 + (int)etv.tv_usec / 1000 - stv.tv_sec * 1000 - (int)stv.tv_usec/1000));

	lua_pushlightuserdata(L, t);

	return 1;
}

static int _real_time(lua_State *L)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	lua_pushnumber(L, tv.tv_sec);
	lua_pushnumber(L, tv.tv_usec);

	return 2;
}

static const luaL_Reg ltlib_f [] = {
	{"create", _create},
	{"delete", _delete},

	{"serialize", _serialize},
	{"size", _size},

	{"convert", _convert},
	{"real_time", _real_time},

	{"ipairs", _ipairs},
	{"pairs", _pairs},

	{NULL, NULL},
};

static const luaL_Reg ltlib_m [] = {
	{"__index", _get},
	{"__newindex", _set},
	
	{NULL, NULL},
};

int init_light_table(lua_State *L)
{
	luaL_newmetatable(L, "ltmeta");
	luaL_register(L, NULL, ltlib_m);
	luaL_register(L, "lighttable", ltlib_f);

	return 0;
}
