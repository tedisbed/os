#ifndef DATA_QUEUE_H
#define DATA_QUEUE_H

#include <queue>
#include <string>
#include <utility>

using namespace std;

class data_queue {
	public:
		int read_from_file(string);
		pair<string, string> get_front();
		void insert(pair<string, string>);
		int size();
		bool is_empty();

	private:
		queue<pair<string, string> > data;
};

#endif
