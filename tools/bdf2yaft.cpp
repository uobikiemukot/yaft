/*
	bdf2yaft: convert bdf to yaft font

	usage: ./bdf2yaft TABLE BDF...
		./bdf2yaft BDF or cat BDF | ./bdf2yaft
*/
#include <fstream>
#include <iostream>

#include <iomanip>
#include <string>
#include <map>
#include <vector>
#include <cmath>

#include "util.cpp"

using namespace std;

int main(int argc, char *argv[])
{
	int i;
	ifstream in;
	glyph_map fonts;
	glyph_map::iterator itr;
	conv_table table;

	if (argc == 1) { /* read from cin */
		in.open("/dev/stdin");
		read_bdf(in, NULL, fonts);
	}
	else if (argc == 2) {
		in.open(argv[1]);
		read_bdf(in, NULL, fonts);
	}
	else {
		in.open(argv[1]);
		read_table(in, table);
		for (i = 2; i < argc; i++) {
			in.open(argv[i]);
			read_bdf(in, &table, fonts);
		}
	}
	
	itr = fonts.begin();
	while (itr != fonts.end()) {
		cout << itr->first << endl;
		dump_glyph(itr->second);
		itr++;
	}
}
