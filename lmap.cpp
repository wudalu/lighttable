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
			
			//ֻ����value�����ü���Ϊ1��ʱ�򣬲����ͷ��ڴ�Ĺ���
			if (n->v.ref == 1) {
				_clear_value(&(n->v));
			}
			//�������ü���
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

//ÿ�β���ڵ��ʱ���ҵ�������ı�ͷ���룬���Ч�� 
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
	//�ҵ���������,����ָ�룬�ҵ���������ĵ�һ���ڵ�ĵ�ַ
	struct node **pn = &data[h];

	//�����ڵ�
	struct node *n = _new_node(key, sz, v->type);

	//������ָ�븳ֵ���������ü������Ӷ�����Ƶ����new/delete�ڴ�
	//_copy_ptr(v, &n->v);
	n->v = *v;
	v->ref += 1;
	
	//ָ��������
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
