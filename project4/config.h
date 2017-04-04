#ifndef CONFIG_H
#define CONFIG_H

#include <map>
#include <string>

using namespace std;

class config {
	public:
		config();
		int parse_config(string); // parse config file
		map<string,string> info;  // dictionary of all values

};
#endif
