#include "config.h"
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>

using namespace std;

config::config(){
	// set default values for the config files
	info["PERIOD_FETCH"] = "180";
	info["NUM_FECTCH"] = "1";
	info["SEARCH_FILE"] = "Search.txt";
	info["NUM_PARSE"] = "1";
	info["SITE_FILE"] = "Sites.txt";
}

int config :: parse_config(string filename){
	ifstream File;
	File.open(filename.c_str());

	// check to make sure file existes
	if(!File){
		cout << "Error: Invalid config file" << endl;
		return -1;
	}

	string data, parsed;


	while(File >> data) {
		for (size_t i = 0; i < data.length(); i++) {
			if(data[i] == '='){ // parse config file
				string a = data.substr(0,i);
				// make sure that the vaiable is accepted
				if(a == "PERIOD_FETCH" || a == "NUM_FETCH" || a == "NUM_PARSE" || a == "SEARCH_FILE" || a == "SITE_FILE"){
					string b = data.substr(i+1, data.length() - i);
					info[a] = b;
				} else {
					// error message if the variable is not accepted
					cout << "Warning: Invalid config parameter " << a << " will be ignored" << endl;
				}
			}
		}
	}

	return 0;
}
