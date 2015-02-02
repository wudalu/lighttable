#include "mem_pool.h"
#include <iostream>
using namespace std;


int main(int argc, char **argv)
{
	memory_pool *instance = memory_pool::get_instance();	
	cout<<instance->get_len()<<endl;

	return 0;
}
