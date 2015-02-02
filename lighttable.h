#ifndef LIGHT_TABLE_H
#define LIGHT_TABLE_H

#include "lmap.h"

#define BUF_SIZE 0x800000

typedef void (*table_setstring_func)(void *ud, const char *str, size_t sz);

struct serialize_buff {
	char *cur;
	char buff[BUF_SIZE];
};

class table {
	public:
		table();
		~table();

		int getref();//��ȡ���ü���
		void grab();//�������ü���
		void _release();//�������ü���
		//TODO
		//�����������Ĺ����Ƿ���Ҫ
		int get_type(const char *key, size_t sz_idx, union table_value *v); //��ȡָ��������type
		void value_string(union table_value *v, table_setstring_func sfunc, void *ud);

		size_t size();//table��size
		size_t keys(struct table_key *tk, size_t cap);//table����key, ����ֵΪkey����Ŀ�����е�key����tk�� 

		//type getter
		double get_number(const char *key, size_t sz_idx);
		int get_boolean(const char *key, size_t sz_idx);
		uint64_t get_ud(const char *key, size_t sz_idx);
		char * get_string(const char *key, size_t sz_idx);

		//type setter
		int set_number(const char *key, size_t sz_idx, double n);
		int set_boolean(const char *key, size_t sz_idx, int b);
		int set_ud(const char *key, size_t sz_idx, uint64_t ud);
		int set_string(const char *key, size_t sz_idx, const char *str, size_t sz);

		//���л��뷴���л�����
		int serialize();//���ط�0��ʾ���󣬷���ɹ�
		const char * unserialize(); //��table���л�Ϊһ��string����
		char *get_serialed();

	private:
		//data members
		int ref;
		int lock;
		int map_lock;
	
		lmap *map;

		//private function members
		//for get,search
		void _search_table(const char *key, size_t sz_idx, struct value *result);
		void _search_map(const char *key, size_t sz_idx, struct value *result);
		//for set, insert
		int _insert_table(const char *key, size_t sz_idx, struct value *v);
		int _insert_map_value(const char *key, size_t sz_idx, struct value *v);

		void _table_lock();
		void _table_unlock();

		//map���ü������
		lmap *_grab_map();
		void _release_map(lmap *m);

		//string���ü������
		struct string_slot *_grab_string(struct lstring *ls);
		void _release_string(struct string_slot *s);

		//string�ӿ�
		void _update_string(struct lstring *str, const char *ustr, size_t sz);

		//map�ӿ�
		void _expand_hash();
		void _update_map(lmap *m);

		void _do_serialize(struct serialize_buff *sb);	
		void _add_string_to_buff(char *str);
};

#endif  //LIGHT_TABLE_H
