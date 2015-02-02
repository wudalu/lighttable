#include "mem_pool.h"

#include <iostream>
#include <string.h>

using namespace std;

const size_t DEFAULT_SIZE = 10;
const size_t BLOCK_SIZE = 512;

memory_pool* memory_pool::instance = NULL;

memory_block::memory_block(size_t init_block_size)
{
	size = init_block_size;
	data = new char[size];
	memset(data, 0, size);
	next = NULL;
}

memory_block::~memory_block()
{
	delete []data;
	data = NULL;
	size = 0;
	next = NULL;
}

memory_pool::memory_pool(size_t init_len, size_t init_block_size)
{
	len = init_len;
	struct memory_block * trace = new struct memory_block(init_block_size);
	head = trace;

	for (int i=1; i<init_len; i++){
		trace->next = new struct memory_block(init_block_size);
		trace = trace->next;
	}
}

memory_pool::~memory_pool()
{
	struct memory_block *trace  = head;
	while (trace != NULL) {
		struct memory_block *temp = trace;
		delete trace;	
		trace = temp->next;
	}
}

memory_pool* memory_pool::get_instance()
{
	if (instance == NULL)
		instance = new memory_pool(DEFAULT_SIZE, BLOCK_SIZE);

	return instance;
}

struct memory_block * memory_pool::get_block()
{
	struct memory_block *ret = head;		
	head = head->next;

	return ret;
}

void memory_pool::free_block(struct memory_block *block)
{
	block->next = head;
	head = block;
}

int memory_pool::get_len()
{
	return len;
}

