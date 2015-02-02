#ifndef MEM_POOL_H
#define MEM_POOL_H

#include <cstddef>

struct memory_block {
	memory_block(size_t size);
	~memory_block();

	size_t size;
	char *data;
	struct memory_block *next;
};


class memory_pool{
	public:
		static memory_pool* get_instance();

		struct memory_block*  get_block();
		void free_block(struct memory_block *block);

		int get_len();

	private:

		memory_pool(size_t init_len, size_t block_size);
		~memory_pool();

		size_t len;
		struct memory_block *head;
		struct memory_block *trace;

		static memory_pool *instance;

};


#endif //MEM_POOL_H

