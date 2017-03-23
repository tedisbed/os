#include <fstream>
#include <iostream>
#include <string>


using namespace std;


void write_to_file(string time, string term, string site, string count, string filename) {
    ofstream fd;
    fd.open(filename.c_str(), ios_base::app);

    if (!fd) {
        cout << "error creating file" << endl;
    }

    fd.seekp(0, ios_base::end);
    if (fd.tellp() == 0) {
        fd << "Time,Phrase,Site,Count" << endl;
    }

    fd << time << "," << term << "," << site << "," << count << endl;
    fd.close();
}
