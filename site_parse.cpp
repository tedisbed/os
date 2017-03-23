#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include "site_parse.h"

using namespace std;

void read_file_data::open_file(string filename){
	ifstream File;
	File.open(filename.c_str());

	if(!File){
		cout << "The file was read in incorrectly, error" << endl;
	}

	string input;

	while(File >> input){
		cout << input << endl;
		data.push_back(input);
	}
	File.close();
}
