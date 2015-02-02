#include "ltutil.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <cmath>

#define GOLDEN 0.6180339887

//JS Hash
//from lua
uint32_t hash_str(const char *name, size_t len, int map_size)
{
	uint32_t h = (uint32_t)len;
	size_t i;
	for (i=0; i<len; i++)
		h = h ^ ((h<<5) + (h>>2) + (uint32_t)name[i]);

	return (h & (map_size - 1));
}

uint32_t mod(size_t numkey, int map_size)
{
	return (numkey % map_size);
}

uint32_t mul(size_t numkey, int map_size)
{
	double val = numkey * GOLDEN;
	return (uint32_t)(val - (int)val) * map_size;
}

uint32_t hash_num(size_t numkey, int map_size)
{
	//return(mod((mod(numkey, map_size) + i * (1+(mod(numkey , (map_size-2))))) , map_size));
	return mod(numkey, map_size);
	//return mul(numkey, map_size);
}

uint32_t hash(const char *name, size_t num, int map_size)
{
	if (name != NULL)
		return hash_str(name, num, map_size);

	return hash_num(num, map_size);
}

int cmp_string(struct string_slot *a, const char *b, size_t sz)
{
	return (a->sz == sz)
		&& memcmp(a->buf, b, sz) == 0;
}

struct string_slot *new_string(const char *name, size_t sz)
{
	struct string_slot *s = (struct string_slot *)malloc(sizeof(*s) + sz);
	s->ref = 1;
	s->sz = sz;
	memcpy(s->buf, name, sz);
	s->buf[sz] = '\0';
	return s;
}

void _clear_value(struct value *v)
{
	table *t = NULL;
	switch (v->type) {
		case LT_STRING:
			free(v->v.s->slot);
			free(v->v.s);
			break;
		case LT_UD:
			t = (table *)(uintptr_t)v->v.ud;
			t->_release();
			break;
	}
}

void _copy_value(struct value *ov, struct value *tv)
{
	table *t;
	switch (ov->type) {
		case LT_NUMBER:
			tv->v.n = ov->v.n;
			break;
		case LT_BOOLEAN:
			tv->v.b = ov->v.b;
			break;
		case LT_UD:
			t = (table *)(uintptr_t)(ov->v.ud);
			t->grab();
			tv->v.ud = ov->v.ud;
			break;
		case LT_STRING:
			struct lstring *ls = (struct lstring *)malloc(sizeof(*ls));
			memset(ls, 0, sizeof(*ls));
			struct string_slot *ss = new_string(ov->v.s->slot->buf, ov->v.s->slot->sz);
			ls->slot = ss;
			tv->v.s = ls;
			break;
	}
}	

void printstr(void *ud, const char *str, size_t sz)
{
	printf("%s\n", str);
}

bool is_prime(unsigned int n)
{
	static unsigned int first[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 0};

	int i;
	for (i=0; first[i] != 0; i++) {//try obvious sizes first
		if ((n % first[i]) == 0) {
			return false;
		}
	}

	int half = (int)sqrt(n);
	for (i=29; i <= half; i++) {
		if (n % i == 0) {
			return false;
		}
	}
	return true;
}

unsigned int next_prime(unsigned int n)
{
	while (!is_prime(++n)) {}

	return n;
}

unsigned int find_double_prime(unsigned int n)
{
	unsigned int k = n / 2;

	while (true) {
		k = next_prime(k);
		
		if (is_prime(k * 2 + 1))
			return (k * 2 + 1);
	}

	return 0;
}
