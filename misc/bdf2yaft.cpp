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

enum {
	NOT_BITMAP = 0,
	IN_BITMAP = 1,
	UCS2_MAX = 0xFFFF,
};

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
