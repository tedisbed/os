#ifndef SITE_PARSE_H
#define SITE_PARSE_H

#include <vector>
#include <string>

using namespace std;

class read_file_data {
	public:
		void open_file(string);
		vector<string> data;
};
#endif
