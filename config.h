#ifndef CONFIG_H
#define CONFIG_H

#include <map>
#include <string>
using namespace std;

class config{
	public:
		config();
		void parse_config(string);
		map<string,string> info;

};
#endif