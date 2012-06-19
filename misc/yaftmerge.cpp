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

enum {
	BITS_PER_BYTE = 8,
};

typedef unsigned char u8;
typedef unsigned short u16;

typedef vector<u8> bitmap_t;
typedef struct glyph_t glyph_t;
typedef map<u16, glyph_t> glyph_map; /* <UCS2, glyph_t> */

struct glyph_t {
	int width, height; /* width, height */
	bitmap_t bitmap;
};

void dump_glyph(glyph_t &glyph)
{
	int i;

	cout << glyph.width << " " << glyph.height << endl;

	for (i = 0; i < glyph.bitmap.size(); i++)
		cout << hex << uppercase << setw(2) << setfill('0') << (int) glyph.bitmap[i] << endl;

	cout.unsetf(ios::hex | ios::uppercase);
}

void read_yaft(char *path, glyph_map &fonts)
{
	int state = 0, count = 0, size;
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
			code = str2int(vec[0]);
			state = 1;
			break;
		case 1:
			glyph.width = str2int(vec[0]);
			glyph.height = str2int(vec[1]);
			size = ceil((double) glyph.width / BITS_PER_BYTE) * glyph.height;
			state = 2;
			break;
		case 2:
			glyph.bitmap.push_back(str2int(vec[0], 16));
			count++;
			if (count >= size) {
				if (fonts.find(code) != fonts.end())
					fonts.erase(code);
				fonts.insert(make_pair(code, glyph));
				count = state = 0;
				glyph.bitmap.clear();
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
