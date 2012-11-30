#include <iostream>
#include <fstream>
#include <map>
#include <string>

#include <curses.h>
#include <term.h>

#include "util.cpp"

using namespace std;

typedef map<string, string> table;

void read_table(table &rom2hira, string &path)
{
	ifstream ifs;
	string str;
	vector<string> vec;

	ifs.open(path.c_str());

	while (getline(ifs, str)) {
		vec = split(str, '\t');

		if (vec.size() != 2)
			continue;

		//cerr << vec[0] << ":" << vec[1] << endl;
		rom2hira.insert(make_pair(vec[0], vec[1]));
	}

	ifs.close();
}

int main()
{
	string str, path = "table";
	char ch;
	table rom2hira;

	read_table(rom2hira, path);

	/*
	while (1) {
		cin.get(ch);
		cout << ch;
	}
	*/

	return 0;
}
