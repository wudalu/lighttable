#include "lmap.h"
#include "ltutil.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctime>
#include <sys/time.h>

#define HALF 0.5

lmap::lmap(size_t n)
{
	ref = 1;
	size = n;
	data = _init_data(n);
	index = 0;
}

lmap::~lmap()
{
	_delete_without_data();
	delete [] data;
	data = NULL;

	size = 0;
	ref = 0;
	index = 0;
}

struct node ** lmap::_init_data(size_t n)
{
	struct node **data = new struct node *[n];	

	for (size_t i=0; i<n; i++) {
		data[i] = NULL;
	}

	return data;
}

void lmap::_delete_without_data()
{
	for (int i=0; i<size; i++) {
		struct node *n = data[i];
		while (n) {
			struct node *next = n->next;
			free(n->strkey);
			
			//只有在value的引用计数为1的时候，才做释放内存的工作
			if (n->v.ref == 1) {
				_clear_value(&(n->v));
			}
			//减少引用计数
			n->v.ref -= 1;

			delete n;
			n = next;
		}
	}
}

struct node * lmap::_new_node(const char *key, size_t sz, int type)
{
	struct node *n = new struct node;
	memset(n, 0, sizeof(*n));
	n->next = NULL;
	//string key
	if (key != NULL) {
		n->key_type = LT_STRING;
		n->strkey = new_string(key, sz);
	}
	//number key
	else {
		n->key_type = LT_NUMBER;
		n->numkey = sz;
	}
	n->v.type = type;

	return n;
}

bool lmap::_is_in_hash(const char *key, size_t sz, struct node *pn)
{
	if ((key != NULL && pn->key_type == LT_STRING && cmp_string(pn->strkey, key, sz)) ||
		(key == NULL && (pn->key_type == LT_NUMBER) &&(pn->numkey == sz))){
			return true;
		}

	return false;
}

void lmap::_search(const char *key, size_t sz, struct value *result)
{
	uint32_t h = hash(key, sz, size);
	struct node *n = data[h];
	while (n) {
		if (_is_in_hash(key, sz, n)) {
			*result = n->v;
			break;
		}
		n = n->next;
	}

	if (n == NULL) {
		result->type = LT_NIL;
	}
}

int lmap::_insert_value(const char *key, size_t sz, struct value *v, int &depth)
{
	uint32_t h = hash(key, sz, size);
	struct node **pn = &data[h];
	while (*pn != NULL) {
		struct node *tmp = *pn;
		if (_is_in_hash(key, sz, tmp)) {
			int type = tmp->v.type;
			tmp->v.v = v->v;
			return type;
		}
		pn = &tmp->next;
		++depth;
	}

	struct node *n = _new_node(key, sz, v->type);
	n->v = *v;
	n->v.ref = 1;
	*pn = n;

	index += 1;

	return LT_NIL;
}

//每次插入节点的时候，找到的链表的表头插入，提高效率 
void lmap::_insert_node(struct string_slot *k, int numkey, struct value *v)
{
	uint32_t h;
	const char *key;
	size_t sz;
	if (k != NULL){
		h = hash(k->buf, k->sz, size);
		key = k->buf;
		sz = k->sz;
	}
	else {
		h = hash(NULL, numkey, size);
		key = NULL;
		sz = numkey;
	}
	//找到所属链表,二级指针，找到的是链表的第一个节点的地址
	struct node **pn = &data[h];

	//创建节点
	struct node *n = _new_node(key, sz, v->type);

	//这里做指针赋值，增加引用计数，从而避免频繁的new/delete内存
	//_copy_ptr(v, &n->v);
	n->v = *v;
	v->ref += 1;
	
	//指针链操作
	n->next = *pn;
	*pn = n;
}

void lmap::_copy_map(lmap *cm)
{
	for (int i=0; i<cm->size; i++) {
		struct node *n = cm->data[i];
		while (n) {
			struct node *next = n->next;
			_insert_node(n->strkey, n->numkey, &n->v);
			n = next;
		}
	}
}

int lmap::_get_ref()
{
	return ref;
}

int lmap::_get_size()
{
	return size;
}

struct node ** lmap::_get_data()
{
	return data;
}

void lmap::_set_ref(int nr)
{
	ref = nr;
}

bool lmap::is_over_half()
{
	return (index / size) >= HALF;
}
