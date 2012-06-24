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

void read_yaft(char *path, glyph_map &fonts)
{
	int state = 0, count = 0;
	ifstream ifs;
	string str;
	vector<string> vec;

	glyph_t glyph;
	u16 code;

	ifs.open(path);
	
	while (getline(ifs, str)) {
		vec = split(str, ' ');
		switch (state) {
		case 0:
			reset_glyph(glyph);
			code = str2int(vec[0]);
			state = 1;
			break;
		case 1:
			glyph.width = str2int(vec[0]);
			glyph.height = str2int(vec[1]);
			state = 2;
			break;
		case 2:
			glyph.bitmap.push_back(str2int(vec[0], 16));
			count++;
			if (count >= glyph.height) {
				if (fonts.find(code) != fonts.end())
					fonts.erase(code);
				fonts.insert(make_pair(code, glyph));
				count = state = 0;
			}
			break;
		default:
			break;
		}
	}

	ifs.close();
}

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
