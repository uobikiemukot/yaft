#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <iomanip>
#include <iostream>
#include <cmath>
#include <fstream>

using namespace std;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

typedef vector<u32> bitmap_t;
typedef struct glyph_t glyph_t;
typedef map<u16, glyph_t> glyph_map; /* <UCS2, glyph_t> */
typedef map<u16, u16> conv_table;

enum {
	CHARS_PER_BYTE = 2,
	BITS_PER_BYTE = 8,
	NOT_BITMAP = 0,
	IN_BITMAP = 1,
	UCS2_MAX = 0xFFFF,
};

struct glyph_t {
	u16 code; /* JIS */
	int width, height; /* width, height */
	bitmap_t bitmap;
};

vector<string> split(const string &str, char delim)
{
	vector<string> res;
	size_t current = 0, found;

	while ((found = str.find_first_of(delim, current)) != string::npos) {
		res.push_back(string(str, current, found - current));
		current = found + 1;
	}
	res.push_back(string(str, current, str.size() - current));

	return res;
}

unsigned int str2int(const string &str, int base = 10)
{
	istringstream is(str);
	unsigned int value;

	switch (base) {
	case 8:
		is >> oct >> value;
	case 10:
		is >> value;
	case 16:
		is >> hex >> value;
	default:
		;
	}

	return value;
}

vector<int> apply_str2int(vector<string> &vec, int base = 10)
{
	vector<string>::iterator itr = vec.begin();
	vector<int> ret;

	while (itr != vec.end()) {
		ret.push_back(str2int(*itr, base));
		itr++;
	}

	return ret;
}

void dump_glyph(glyph_t &glyph)
{
	int i, wide;

	cout << glyph.width << " " << glyph.height << endl;
	wide = ceil((double) glyph.width / BITS_PER_BYTE) * CHARS_PER_BYTE;

	for (i = 0; i < glyph.bitmap.size(); i++)
		cout << hex << uppercase << setw(wide) << setfill('0') << glyph.bitmap[i] << endl;

	cout.unsetf(ios::hex | ios::uppercase);
}

void reset_glyph(glyph_t &glyph)
{
	glyph.code = glyph.width = glyph.height = 0;
	glyph.bitmap.clear();
	
}

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

void read_table(ifstream &in, conv_table &table)
{
	string str;
	vector<string> vec;
	vector<int> pair;

	while (getline(in, str)) {
		/* lines starting from '#' are comment */
		if (str[0] == '#' || str.length() == 0)
			continue;

		vec = split(str, '\t');
		if (vec.size() > 1) {
			pair = apply_str2int(vec, 16);
			table.insert(make_pair(pair[0], pair[1]));
		}
	}

	in.close();
}

void read_bdf(ifstream &in, conv_table *table, glyph_map &fonts)
{
	int i, state = NOT_BITMAP;
	string str;
	vector<string> vec;
	u16 key;

	conv_table::iterator itr;
	glyph_t glyph;

	while (getline(in, str)) {
		vec = split(str, ' ');
		if (vec[0] == "ENCODING") {
			reset_glyph(glyph);
			glyph.code = str2int(vec[1]);
		}
		else if (vec[0] == "DWIDTH")
			glyph.width = str2int(vec[1]);
		else if (vec[0] == "BITMAP") {
			state = IN_BITMAP;
			continue;
		}
		else if (vec[0] == "ENDCHAR") {
			state = NOT_BITMAP;
			if (table == NULL)
				key = glyph.code;
			else if ((itr = table->find(glyph.code)) != table->end())
				key = itr->second;
			else
				continue;

			if (key > UCS2_MAX)
				continue;

			if (fonts.find(key) != fonts.end())
				fonts.erase(key);

			fonts.insert(make_pair(key, glyph));
		}

		if (state == IN_BITMAP) {
			glyph.bitmap.push_back(str2int(vec[0], 16));
			glyph.height++;
		}
	}

	in.close();
}
