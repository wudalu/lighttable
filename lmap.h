#ifndef LMAP_H
#define LMAP_H

#include "lstruct.h"

#define DEFAULT_DEPTH 1

struct node  {
	struct node *next;
	int key_type;//number or string
	struct string_slot *strkey;
	int numkey;
	struct value v;
};

class lmap {
	public:
		lmap(size_t n);
		~lmap();

		struct node ** _init_data(size_t n);
		void _delete_without_data();
		void _search(const char *key, size_t sz_idx, struct value *v);
		int _insert_value(const char *key, size_t sz_idx, struct value *v, int &depth);
		void _copy_map(lmap *cm);		

		int _get_ref();
		int _get_size();
		struct node ** _get_data();

		void _set_ref(int nf);
		bool is_over_half();
	private:
		int ref;
		int size;
		struct node **data;
		int index;
		//struct node *n[1];

		//private functions
		struct node * _new_node(const char *key, size_t sz, int type);
		void _insert_node(struct string_slot *k, int numkey, struct value *v);
		bool _is_in_hash(const char *key, size_t sz, struct node *pn);

};

#endif //LMAP_H
