#include "lighttable.h"
#include "ltutil.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <inttypes.h>
#include <ctime>
#include <sys/time.h>

#define DEFAULT_SIZE 4
#define MAX_HASH_DEPTH 7 

//存放序列化后字符串
struct serialize_buff serialed;

table::table()
{
	ref = 1;
	lock = 0;
	map_lock = 0;

	int map_size = next_prime(DEFAULT_SIZE);
	map = new lmap(map_size);
}

table::~table()
{
	ref = 0;
	lock = 0;
	map_lock = 0;

	delete map;
	map = NULL;
}

void table::_release()
{
	if (__sync_sub_and_fetch(&ref, 1) != 0)
		return ;

	delete this;
}

void table::grab()
{
	__sync_add_and_fetch(&ref, 1);
}

int table::getref()
{
	return ref;
}

void table::_table_lock()
{
	while (__sync_lock_test_and_set(&lock, 1)) {}
}

void table::_table_unlock()
{
	__sync_lock_release(&lock);
}

lmap * table::_grab_map()
{
	while (__sync_lock_test_and_set(&map_lock, 1)) {}

	int map_ref = map->_get_ref();
	int ref = __sync_add_and_fetch(&map_ref, 1);
	assert(ref > 1);
	lmap *ret = this->map;
	map->_set_ref(ref);
	__sync_lock_release(&map_lock);

	return ret;
}
 
void table::_release_map(lmap *m)
{
	int map_ref = m->_get_ref();
	int ref = __sync_sub_and_fetch(&map_ref, 1);
	map->_set_ref(ref);
	if (ref != 0)
		return;

	delete m;
}

void table::_search_map(const char *key, size_t sz, struct value *result)
{
	lmap *m;
	do {
		m = _grab_map();

		m->_search(key, sz, result);

		_release_map(m);

	} while(m != this->map);
}

void table::_search_table(const char *key, size_t sz_idx, struct value *result)
{
	_search_map(key, sz_idx, result);
}

int table::get_type(const char *key, size_t sz_idx, union table_value *tv)
{
	struct value tmp;
	_search_table(key, sz_idx, &tmp);
	if (tv)
		memcpy(tv,  &(tmp.v), sizeof(*tv));

	return tmp.type;
}

struct string_slot * table::_grab_string(struct lstring *s)
{
	while (__sync_lock_test_and_set(&s->lock, 1)) {}

	int ref = __sync_add_and_fetch(&s->slot->ref, 1);
	assert(ref > 1);
	struct string_slot *ret = s->slot;
	__sync_lock_release(&s->lock);

	return ret;
}

void table::value_string(union table_value *v, void (*sfunc)(void *ud, const char *str, size_t sz) , void *ud)
{
	struct string_slot *s = _grab_string((struct lstring *)v->p);	
	sfunc(ud, s->buf, s->sz);
	_release_string(s);
}

void table::_release_string(struct string_slot *s)
{
	if (__sync_sub_and_fetch(&s->ref, 1) != 0)
		return ;

	free(s);
	s = NULL;
}

size_t table::size()
{
	if (map == NULL) 
		return 0;

	size_t s = 0;

	lmap *m = _grab_map();
	if (m) {
		s += (m->_get_size()) * MAX_HASH_DEPTH;
	}
	_release_map(m);	

	return s;
}

/*
 *这函数太重要了
 *返回值其实是两个
 *count是table真实的value的个数
 *另一个返回值是vv
 *vv:保存所有的table_key
 *map的key的顺序是指针数组的idx
 */
size_t table::keys(struct table_key *vv, size_t cap)
{
	if (map == NULL) {
		return 0;
	}

	size_t count = 0;

	lmap *m = _grab_map();
	for (int i=0; i<m->_get_size(); i++){
		struct node **mdata = m->_get_data();
		struct node *n = mdata[i];
		while (n) {
			if (count >= cap-1) {
				_release_map(m);
				return count;
			}

			++count;
			int record_index = count - 1;
			vv[record_index].type = n->v.type;
			if (n->key_type == LT_STRING) {
				vv[record_index].key = n->strkey->buf;
				vv[record_index].sz_idx = n->strkey->sz;
			}
			else {
				vv[record_index].key = NULL;
				vv[record_index].sz_idx = n->numkey;
			}
			n = n->next;
		}
	}
	_release_map(m);

	return count;
}

//now setters and getters
double table::get_number(const char *key, size_t sz_idx)
{
	struct value tmp;
	_search_table(key, sz_idx, &tmp);	
	assert(tmp.type == LT_NIL || tmp.type == LT_NUMBER);

	return tmp.v.n;
}

int table::get_boolean(const char *key, size_t sz_idx)
{
	struct value tmp;
	_search_table(key, sz_idx, &tmp);
	assert(tmp.type ==LT_NIL || tmp.type == LT_BOOLEAN);

	return tmp.v.b;
}

uint64_t table::get_ud(const char *key, size_t sz_idx)
{
	struct value tmp;
	_search_table(key, sz_idx, &tmp);
	assert(tmp.type == LT_NIL || tmp.type == LT_UD);

	return tmp.v.ud;
}

char * table::get_string(const char *key, size_t sz_idx)
{
	char *ret;
	struct value tmp;
	_search_table(key, sz_idx, &tmp);
	if (tmp.type == LT_STRING) {
		struct string_slot *s = _grab_string(tmp.v.s);
		ret = s->buf;
		_release_string(s);
		return ret;
	}
	else {
		assert(tmp.type == LT_NIL);
		return "";
	}
}

void table::_update_map(lmap *m)
{
	while (__sync_lock_test_and_set(&map_lock, 1)) {}

	lmap *old = map;
	map = m;
	int old_ref = old->_get_ref();
	int ref = __sync_sub_and_fetch(&old_ref, 1);
	__sync_lock_release(&map_lock);

	if (ref == 0) {
		delete old;
	}
}

void table::_expand_hash()
{
	struct timeval stv;
	gettimeofday(&stv, NULL);
	lmap *old = this->map;
	int old_size = old->_get_size();
	//int new_size = next_prime(old_size);
	int new_size = old_size * 2;
	printf("newsize=%d\n",new_size);
	lmap *m = new lmap(new_size);
	struct timeval ntv;
	gettimeofday(&ntv, NULL);
	printf("new time=%d\n",(ntv.tv_sec * 1000 + ntv.tv_usec / 1000)-(stv.tv_sec * 1000 + stv.tv_usec / 1000));

	//链表复制
	m->_copy_map(old);
	struct timeval etv;
	gettimeofday(&etv, NULL);

	printf("copy time=%d\n",(etv.tv_sec * 1000 + etv.tv_usec / 1000)-(ntv.tv_sec * 1000 + ntv.tv_usec / 1000));
	_update_map(m);
}

int table::_insert_map_value(const char *key, size_t sz, struct value *v)
{
	int depth = 0;

	int ret = map->_insert_value(key, sz, v, depth);
	
	//if (depth >= MAX_HASH_DEPTH && map->is_over_half()) {
	if (depth >= MAX_HASH_DEPTH){
		printf("fafjaljlfjalfjalfj\n");
		_expand_hash();
	}

	return ret;
}

int table::_insert_table(const char *key, size_t sz_idx, struct value *v)
{
	_table_lock();
	int type = _insert_map_value(key, sz_idx, v);
	_table_unlock();

	return type;
}

//返回0表示成功，非0反之
int table::set_number(const char *key, size_t sz_idx, double n)
{
	struct value tmp;
	tmp.type = LT_NUMBER;
	tmp.v.n = n;
	int type = _insert_table(key, sz_idx, &tmp);

	return type != LT_NUMBER && type != LT_NIL;
}

int table::set_boolean(const char *key, size_t sz_idx, int b)
{
	struct value tmp;
	tmp.type = LT_BOOLEAN;
	tmp.v.b = b;
	int type = _insert_table(key, sz_idx, &tmp);

	return type != LT_BOOLEAN && type != LT_NIL;
}

int table::set_ud(const char *key, size_t sz_idx, uint64_t ud)
{
	struct value tmp;
	tmp.type = LT_UD;
	tmp.v.ud = ud;
	int type = _insert_table(key, sz_idx, &tmp);

	return type != LT_UD && type != LT_NIL;
}

void table::_update_string(struct lstring *str, const char *ustr, size_t sz)
{
	struct string_slot *ns = new_string(ustr, sz);
	
	while (__sync_lock_test_and_set(&str->lock, 1)) {}

	struct string_slot *old = str->slot;
	str->slot = ns;
	int ref = __sync_sub_and_fetch(&old->ref, 1);
	__sync_lock_release(&str->lock);

	if (ref == 0)
		delete old;
}

int table::set_string(const char *key, size_t sz_idx, const char *str, size_t sz)
{
	struct value tmp;
	_search_table(key, sz_idx, &tmp);
	if (tmp.type == LT_STRING) {
		_update_string(tmp.v.s, str, sz);
		return 0;
	}

	if (tmp.type != LT_NIL) 
		return 1;
	
	struct lstring *ls = (struct lstring *)malloc(sizeof(*ls));
	memset(ls, 0, sizeof(ls));
	ls->slot = new_string(str, sz);
	tmp.type = LT_STRING;
	tmp.v.s = ls;
	
	_insert_table(key, sz_idx, &tmp);

	return 0;
}

char *table::get_serialed()
{
	return serialed.buff;
}

void table::_add_string_to_buff(char *str)
{
	while (*str && serialed.cur < (serialed.buff + BUF_SIZE)) {
		*(serialed.cur)++ = *str++;
	}
}

void table::_do_serialize(struct serialize_buff *serialed)
{
	size_t size = this->size();
	struct table_key *keys = (struct table_key *)malloc(size * sizeof(*keys));
	size = this->keys(keys, size);

	char *strret;
	char *temp = new char[1000];
	_add_string_to_buff("{");
	for (int i=0; i<size; i++) {
		memset(temp, 0, sizeof(*temp));
		if (keys[i].key == NULL) {
			if (keys[i].type != -1) {
				sprintf(temp, "[\"%d\"]=", keys[i].sz_idx);
				_add_string_to_buff(temp);
			}
		}
		else {
				sprintf(temp, "[\"%s\"] = ", keys[i].key);
				_add_string_to_buff(temp);
		}

		memset(temp, 0, sizeof(*temp));
		switch (keys[i].type) {
			case LT_NIL:
				break;
			case LT_NUMBER: {
								double d = get_number(keys[i].key, keys[i].sz_idx);		
								sprintf(temp, "%lf", d);
								_add_string_to_buff(temp);
								break;
							}
			case LT_BOOLEAN: {
								 int b = get_boolean(keys[i].key, keys[i].sz_idx);
								 sprintf(temp, "%s",b ? "true" : "false");
								_add_string_to_buff(temp);
								 break;
							 }
			case LT_UD: {
							sprintf(temp, "{");
							table * sub = (table *)get_ud(keys[i].key, keys[i].sz_idx);
							sub->_do_serialize(serialed);
							sprintf(temp, "}");
							break;
						}
			case LT_STRING: {
						strret = get_string(keys[i].key, keys[i].sz_idx);
						sprintf(temp, "\"%s\"", strret);
						_add_string_to_buff(temp);
						break;
							}
			default:
						   //assert(0);
						   break;
		}
		if (keys[i].type != -1) {
			_add_string_to_buff(",\n");
		}
	}
	_add_string_to_buff("}");
	delete [] temp;
	temp = NULL;
	free(keys);
	keys = NULL;
}

int table::serialize()
{
	serialed.cur  = serialed.buff;
	_do_serialize(&serialed);
	printf("\nresult=%s\n", serialed.buff);
	return 0;
}

const char * table::unserialize()
{
	return "";
}
