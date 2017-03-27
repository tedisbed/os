#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include "data_queue.h"

using namespace std;

void data_queue::read_from_file(string filename) {
	ifstream File;
	File.open(filename.c_str());

	if(!File){
		cout << "Error. Invalid file name." << endl;
	}

	string input;

	while(File >> input){
		cout << input << endl;
		data.push(input);
	}
	File.close();
}

string data_queue::get_front() {
	string ret_data = data.front();
	data.pop();
	return ret_data;
}

void data_queue::insert(string new_string) {
	data.push(new_string);
}

bool data_queue::is_empty() {
	return data.empty();
}
