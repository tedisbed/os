#include <string>
#include <utility>
#include <vector>
#include <fstream>
#include <iostream>
#include "data_queue.h"

using namespace std;

int data_queue::read_from_file(string filename) {
	ifstream File;
	File.open(filename.c_str());

	if(!File){
		cout << "Error. Invalid sites file" << endl;
		return -1;
	}

	string input;

	while(File >> input) {
		pair<string, string> input_pair = make_pair(input, "");
		data.push(input_pair);
	}

	File.close();
	return 0;
}

pair<string, string> data_queue::get_front() {
	pair<string, string> ret_data = data.front();
	data.pop();
	return ret_data;
}

void data_queue::insert(pair<string, string> new_pair) {
	data.push(new_pair);
}

bool data_queue::is_empty() {
	return data.empty();
}
