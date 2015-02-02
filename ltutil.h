#ifndef LTUTIL_H
#define LTUTIL_H
#include "lighttable.h"
//extern struct string_slot;

uint32_t hash(const char *name, size_t len, int map_size);
int cmp_string(struct string_slot *a, const char *b, size_t sz);
struct string_slot *new_string(const char *name ,size_t sz);
void _clear_value(struct value *v);
void _copy_value(struct value *ov, struct value *tv);
void printstr(void *ud, const char *str, size_t sz);
bool is_prime(unsigned int n);
unsigned int next_prime(unsigned int n);
unsigned int find_double_prime(unsigned int n);

#endif //LTUTIL_H
