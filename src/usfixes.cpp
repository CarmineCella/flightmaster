// usfixes.cpp
// 

#include <stdexcept>
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>

#include "utilities.h"

using namespace std;

vector<int> get_GPS_minutes (const std::string id) {
	vector<int> coord;
	string line;
	stringstream line_stream;
	line_stream << id;
	while (!line_stream.eof ()) {
		getline (line_stream, line, '-');
		coord.push_back (atol (line.c_str ()));
	}

	if (coord.size () != 3) throw runtime_error ("invalid format for coordinates");
	return coord;
}
int main (int argc, char* argv[]) {
	try {
		cout << BOLDYELLOW << "[usfixes, ver. " << VERSION << "]" << endl << endl;
		cout << RESET << "US fixes DB generation from FAA data" << endl;
		cout << "source: https://www.faa.gov/air_traffic/flight_info/aeronav/aero_data/NASR_Subscription/" << endl;
		cout << "(c) 2020, www.carminecella.com" << endl << endl;

		if (argc != 3) {
			throw runtime_error ("syntax is: 'usfixes FIX.txt database.csv");
		}

		ifstream input (argv[1]);
		if (!input.good ()) {
			throw runtime_error ("cannot open input file");
		}

		ofstream output (argv[2]);
		if (!output.good ()) {
			throw runtime_error ("cannot create output file");
		}
		output << "\"ident\",  \"country\", \"latitude_deg\", \"longitude_deg\"" << endl;
		int linenum = 1;
		cout << "converting..." << endl; cout.flush ();
		while (!input.eof ()) {
			string line;
			stringstream line_str;
			vector<string> tokens;
			string token;
			getline (input, line);
			if (line.find ("FIX1") == string::npos) continue;
			line_str << line;
			while (!line_str.eof ()) {
				line_str >> token;
				tokens.push_back (token);
			}
			if (tokens.size () < 4) {
				cout << "invalid line n. " << linenum << endl;
				continue;
			}
			if (tokens[1] == "RUSSIAN") break; // discard all non-US - assumes order in data
			if (tokens[1] == "ALASKA" ||
				tokens[1] == "DIST." ||
				tokens[1] == "GUAM" ||
				tokens[1] == "HAWAII" ||
				tokens[1] == "MIDWAY" ||
				tokens[1] == "N" ||
				tokens[1] == "OFFSHORE") continue; // do not insert some fixes in US (format problems)
			string wpt = tokens[0].substr (4, tokens[0].size () - 4);
			string country = tokens[1];
			string lat = "";
			string longit = "";
			if (tokens[2][0] == 'K') {
				lat =  tokens[2].substr (2, tokens[2].size () - 3); // discard K? and final N ;
				longit = tokens[3].substr (0, tokens[3].size () - 4); // discard final W???
			} else {
				if (tokens.size () < 5) {
					cout << "invalid line n. " << linenum << endl;
					continue;
				}
				country += " " + tokens[2];		
				lat = tokens[3].substr (2, tokens[3].size () - 3); // discard K? and final N 
				longit = tokens[4].substr (0, tokens[4].size () - 4); // discard final W???;
			}

			vector<int> lat_v = get_GPS_minutes (lat);
			vector<int> long_v = get_GPS_minutes (longit); // all assumed to be W
			//DD = d + (min/60) + (sec/3600)
			double dd_lat = lat_v[0] + ((double) lat_v[1] / 60) + ((double) lat_v[2] / 3600);
			double dd_long = long_v[0] + ((double) long_v[1] / 60) + ((double) long_v[2] / 3600);

			output << wpt << ", " << country << ", " << dd_lat << ", " << -dd_long << endl;
			cout << wpt << ", " << country << ", " << dd_lat << ", " << -dd_long << endl;
			++linenum;
		}	
		cout << "completed" << endl;
	}
	catch (exception& e) {
		cout << "Error: " << e.what () << endl;
	}
	catch (...) {
		cout << "Fatal error: unknown exception" << endl;
	}
	return 0;
}

// EOF

