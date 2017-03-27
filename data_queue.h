#ifndef DATA_QUEUE_H
#define DATA_QUEUE_H

#include <queue>
#include <string>

using namespace std;

class data_queue {
	public:
		void read_from_file(string);
		string get_front();
		void insert(string);
		bool is_empty();

	private:
		queue<string> data;
};

#endif
