// OS Project 4
// Teddy Brombach and Tristan Mitchell


// Import Modules

#include "config.h"
#include "data_queue.h"
#include <ctime>
#include <map>
#include <set>
#include <utility>
#include <vector>
#include <string>
#include <fstream>
#include <signal.h>
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <curl/curl.h>

using namespace std;


// Function & Struct Prototypes

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);
string curl(string filename);
void * fetch(void *args);
void * parse(void *args);
void write_to_file(time_t now, data_queue &data, int counter);

struct MemoryStruct {
	char *memory;
	size_t size;
};


// Global Variables

pthread_mutex_t lock;
pthread_cond_t condp;
int parse_count = 0;
int total_parse = 0;
data_queue fetch_queue, parse_queue, write_queue;
set<string> words;


// Main Execution

int main(int argc, char *argv[]) {
	if (argc != 2) {
		cout << "Error: Invalid command.\nCorrect usage: ./site-tester [config_file]" << endl;
		return -1;
	}

	config a;
	int status = a.parse_config(argv[1]);
	if (status == -1) {
		return status;
	}

	pthread_mutex_init(&lock, NULL);
	pthread_cond_init(&condp, NULL);

	int num_fetch_threads = atoi(a.info["NUM_FETCH"].c_str());
	int num_parse_threads = atoi(a.info["NUM_PARSE"].c_str());
	pthread_t fetch_threads[num_fetch_threads];
	pthread_t parse_threads[num_parse_threads];

	ifstream phrase_file;
	phrase_file.open(a.info["SEARCH_FILE"].c_str());
	string word;
	while (phrase_file >> word) {
		words.insert(word);
	}

	int counter = 1;

	while (1) {
		status = fetch_queue.read_from_file(a.info["SITE_FILE"]);
		if (status == -1) {
			return status;
		}
		if (fetch_queue.size() < num_parse_threads) {
			num_parse_threads = fetch_queue.size();
			cout << "Warning: Reducing number of fetch threads to match number of sites" << endl;
		}
		if (fetch_queue.size() < num_fetch_threads) {
			num_fetch_threads = fetch_queue.size();
			cout << "Warning: Reducing number of parse threads to match number of sites" << endl;
		}
		total_parse = fetch_queue.size();

		for (int i = 0; i < num_fetch_threads; i++) {
			pthread_create(&fetch_threads[i], NULL, fetch, NULL);
		}
		for (int j = 0; j < num_parse_threads; j++) {
			pthread_create(&parse_threads[j], NULL, parse, NULL);
		}

		for (int k = 0; k < num_fetch_threads; k++) {
			pthread_join(fetch_threads[k], NULL);
		}
		for (int m = 0; m < num_parse_threads; m++) {
			pthread_join(parse_threads[m], NULL);
		}

		time_t now = time(0);
		write_to_file(now, write_queue, counter);
		counter++;
		parse_count = 0;
		cout << "Definetly a write error" << endl;
		usleep(1000000 * atoi(a.info["PERIOD_FETCH"].c_str()));
		//signal(SIGALARM, handle_alarm);
	}
}


// Function Definitions

/*void handle_alarm(int x){

  }*/

void * fetch(void *ptr) {
	// need condition variable here
	cout << "in fetch function" << endl;
	while (1) {
		pthread_mutex_lock(&lock);
		if (fetch_queue.is_empty()) {
			pthread_mutex_unlock(&lock);
			break;
		}
		cout << "in fetch while loop" << endl;

		string site_name = fetch_queue.get_front().first;
		pthread_mutex_unlock(&lock);

		string data = curl(site_name);
		pair<string, string> data_pair = make_pair(site_name, data);

		pthread_mutex_lock(&lock);
		parse_queue.insert(data_pair);
		pthread_cond_signal(&condp);
		pthread_mutex_unlock(&lock);
	}
	cout << "done with fetch" << endl;
}

void * parse(void *ptr) {
	cout << "in parse function" << endl;

	while (parse_queue.is_empty()) {
		cout << "Waiting for cond wait" << endl;
		pthread_cond_wait(&condp, &lock);
		pthread_mutex_unlock(&lock);
	}
	while (1) {
		cout << "in parse while loop" << endl;
		pthread_mutex_lock(&lock);
		if (parse_queue.is_empty()) {
			pthread_mutex_unlock(&lock);
			break;
		}
		pair<string, string> input_pair = parse_queue.get_front();
		pthread_mutex_unlock(&lock);

		string site_name = input_pair.first;
		string data_string = input_pair.second;
		map<string, int> count_dict;
		set<string>::iterator it;

		for (it = words.begin(); it != words.end(); it++) {
			cout << "in parse for loop" << endl;
			size_t pos = data_string.find(*it, 0);
			int count = 0;
			while (pos != string::npos) {
				count = count + 1;
				pos = data_string.find(*it, pos + 1);
			}
			count_dict[*it] = count;
		}

		map<string, int>::iterator it2;
		for (it2 = count_dict.begin(); it2 != count_dict.end(); it2++) {
			int num = it2->second;
			stringstream conversion_stream;
			conversion_stream << num;
			string num_string = conversion_stream.str();
			string output_string = it2->first + "," + site_name + "," + num_string;
			pair<string, string> output_pair = make_pair(output_string, "");

			pthread_mutex_lock(&lock);
			write_queue.insert(output_pair);
			parse_count++;
			pthread_mutex_unlock(&lock);
		}

		pthread_mutex_lock(&lock);
		if (parse_count < total_parse && parse_queue.is_empty()) {
			pthread_cond_wait(&condp, &lock);
			pthread_mutex_unlock(&lock);
			continue;
		}
		pthread_mutex_unlock(&lock);
	}
}

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)userp;

	mem->memory = (char*) realloc(mem->memory, mem->size + realsize + 1);
	if (mem->memory == NULL) {
		/* out of memory! */
		printf("not enough memory (realloc returned NULL)\n");
		return 0;
	}

	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

string curl(string filename) {
	CURL *curl_handle;
	CURLcode res;

	struct MemoryStruct chunk;

	chunk.memory = (char*) malloc(1);  /* will be grown as needed by the realloc above */
	chunk.size = 0;    /* no data at this point */

	curl_global_init(CURL_GLOBAL_ALL);

	/* init the curl session */
	curl_handle = curl_easy_init();

	/* specify URL to get */
	curl_easy_setopt(curl_handle, CURLOPT_URL, filename.c_str());

	/* send all data to this function  */
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

	/* we pass our 'chunk' struct to the callback function */
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

	/* some servers don't like requests that are made without a user-agent
	   field, so we provide one */
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

	/* get it! */
	res = curl_easy_perform(curl_handle);

	/* check for errors */
	if(res != CURLE_OK) {
		fprintf(stderr, "curl_easy_perform() failed: %s\n",
				curl_easy_strerror(res));
	}

	string result = chunk.memory;

	/* cleanup curl stuff */
	curl_easy_cleanup(curl_handle);
	free(chunk.memory);

	/* we're done with libcurl, so clean it up */
	curl_global_cleanup();

	return result;
}

void write_to_file(time_t now, data_queue &data, int counter) {
	stringstream conversion_stream;
	conversion_stream << counter;
	string counter_string = conversion_stream.str();

	string filename = counter_string + ".csv";
	ofstream fd;
	fd.open(filename.c_str(), ios_base::out);

	if (!fd) {
		cout << "Error creating output file " << filename << endl;
		return;
	}

	fd.seekp(0, ios_base::end);
	if (fd.tellp() == 0) {
		fd << "Time,Phrase,Site,Count" << endl;
	}

	struct tm *t_now = localtime(&now);
	char buffer[20];
	strftime(buffer, 20, "%F-%T", t_now);

	while (!data.is_empty()) {
		string data_string = data.get_front().first;
		fd << buffer << "," << data_string << endl;
	}
	if(!data.is_empty()){
		cout << "There is data here" << endl;
	}

	fd.close();
}
