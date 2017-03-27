#include "config.h"
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>

using namespace std;

config::config(){
	info["PERIOD_FETCH"] = "180";
	info["NUM_FECTCH"] = "1";
	info["SEARCH_FILE"] = "Search.txt";
	info["NUM_PARSE"] = "1";
	info["SITE_FILE"] = "Sites.txt";
}

void config :: parse_config(string filename){
	// set the default values
	ifstream File;
	File.open(filename.c_str());

	if(!File){
		cout << "The config file was not opened correctly" << endl;
		return;
	}

	string data, parsed;

	while(File >> data){
		for (size_t i = 0; i < data.length(); i++) {
			if(data[i] == '='){
				string a = data.substr(0,i);
				if(a == "PERIOD_FETCH" || a == "NUM_FETCH" || a == "NUM_PARSE" || a == "SEARCH_FILE" || a == "SITE_FILE"){
					string b = data.substr(i+1, data.length() - i);
					info[a] = b;
				}
			}
		}
	}
}
