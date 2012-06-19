#include <sstream>
#include <string>
#include <vector>

using namespace std;

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

int str2int(const string &str, int base = 10)
{
	istringstream is(str);
	int value;

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
