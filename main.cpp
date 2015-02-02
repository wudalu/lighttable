#include "ltutil.h"
#include "lighttable.h"

#include <string.h>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>

using namespace std;

/*
void printstr(void *ud, const char *str, size_t sz)
{
	printf("%s\n", str);
}
*/

int main(int argc, char **argv)
{
	table *t = new table;
	/*
	t->set_number("a", 2, 2);
	cout<<"number = "<<t->get_number("a",2)<<endl;
	t->set_boolean("b", 2, 1);
	t->set_id("id", 3, 99999);
	t->set_id("id1", 4, 79999);
	cout<<"boolean = "<<t->get_boolean("b",2)<<endl;
	cout<<"id = "<<t->get_id("id",3)<<endl;
	cout<<"size="<<t->size()<<endl;

	for (int i=0; i<100; i++) {
		t->set_number(NULL, i, i);
		cout<<"array value "<<i<<"is"<<t->get_number(NULL, i)<<endl;
	}
	*/

	t->set_string("str", 4,"valuestr",strlen("valuestr"));
	t->get_string("str", 4); 

	/*
	table *sub = new table;
	sub->set_table("tbl",3, t);
	for (int i=0; i<100; i++) {
		sub->set_number(NULL, i, i);
	}
	cout<<"sub table size="<<sub->size()<<endl;
	table *ret = sub->get_table("tbl", 3);
	cout<<"ret tbl info, size="<<ret->size()<<"number ="<<ret->get_number("a",1)<<endl;


	struct table_key *tk = new struct table_key[140];
	size_t osize = sub->size();
	size_t count =sub->keys(tk, osize);
	for (int i=0; i<140; i++) {
		//printf("table_key array, key=%s\n", tk[i].key);
		//printf("table_key array, sz_idx=%d\n", tk[i].sz_idx);
	}

	delete t;
	delete [] tk;
	delete sub;
	 */
	delete t;
	
	return 0;
}
