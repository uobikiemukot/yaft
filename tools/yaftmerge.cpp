/*
	yaftmerge: merge several yaft fonts

	usage: ./yaftmerge FILE...
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
	glyph_map fonts;
	glyph_map::iterator itr;

	if (argc < 2) {
		cerr << "usage: ./yaftmerge FILE..." << endl;
		return 1;
	}

	for (i = 1; i < argc; i++)
		read_yaft(argv[i], fonts);
	
	itr = fonts.begin();
	while (itr != fonts.end()) {
		cout << itr->first << endl;
		dump_glyph(itr->second);
		itr++;
	}
}
