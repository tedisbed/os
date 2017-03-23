#include "config.h"
#include "site_parse.h"
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

using namespace std;
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);
string curl(string filename);
void parse( string, string);

struct MemoryStruct {
  char *memory;
  size_t size;
};

int main(){
	// read in sites.txt
	// can be used for search words file
	config a;
	a.parse_config("param.txt");
	read_file_data words, sites;
	words.open_file(a.info["SEARCH_FILE"]);
	sites.open_file(a.info["SITE_FILE"]);
	vector<string> site_data;
	cout << sites.data.size() << endl;
	for(int i = 0; i< sites.data.size(); i ++){
		site_data.push_back(curl(sites.data[0]));
	}
	for( int i =0;i< site_data.size(); i++){
		for(int j =0;j<words.data.size();j++){
			parse(site_data[i],words.data[j]);
		}
	}
}

static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;
 
  mem->memory = (char*) realloc(mem->memory, mem->size + realsize + 1);
  if(mem->memory == NULL) {
    /* out of memory! */ 
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }
 
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;
 
  return realsize;
}

string curl(string filename)
{
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
  else {
    /*
     * Now, our chunk.memory points to a memory block that is chunk.size
     * bytes big and contains the remote file.
     *
     * Do something nice with it!
     */ 
 
    printf("%lu bytes retrieved\n", (long)chunk.size);
  }

  printf("%s\n",chunk.memory);
  string result = chunk.memory;

 
  /* cleanup curl stuff */ 
  curl_easy_cleanup(curl_handle);
 
  free(chunk.memory);
 
  /* we're done with libcurl, so clean it up */ 
  curl_global_cleanup();

  return result;
}

void parse(string data, string word){
	size_t pos = 0;
	int count = 0;

	while((pos = data.find(word,pos)) != string::npos){
		count = count + 1;
		pos = pos + word.length();
	}
	cout << count << endl;
}
