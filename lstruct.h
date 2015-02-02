#ifndef LSTRUCT_H
#define LSTRUCT_H

#include <stdint.h>
#include <stddef.h>

//types
#define LT_NIL       0
#define LT_NUMBER    1
#define LT_BOOLEAN   2
#define LT_UD        3
#define LT_STRING    4
#define LT_TABLE     5

class table;

struct table_key {
	int type;//���ͣ�number or string
	const char *key; //if type==string, then key name
	size_t sz_idx;   //key index, �����������е�����
};

union table_value {
	double n;
	int b;
	uint64_t ud;
	void *p; //ͨ��ָ�����ͣ��п�����string���п�����lua table
};

struct string_slot {
	int ref;
	int sz;
	char buf[1];
};

struct lstring {
	int lock;
	struct string_slot *slot;
};

struct value {
	int ref; //���ü���
	int type;
	union {
		double n;
		int b;
		uint64_t ud;
		struct lstring *s;
	} v;
};

#endif //LSTRUCT_H
