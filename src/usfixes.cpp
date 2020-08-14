// usfixes.cpp
// 

#include <stdexcept>
#include <iostream>

#include "utilities.h"

using namespace std;

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
			}
			string wpt = tokens[0].substr (4, tokens[0].size () - 4);
			string country = tokens[1];
			string lat = "";
			string longit = "";
			if (tokens[2][0] == 'K') {
				lat =  tokens[2];
				string longit = tokens[3];
			} else {
				if (tokens.size () < 5) {
					cout << "invalid line n. " << linenum << endl;
				}
				country += " " + tokens[2];		
				lat = tokens[3];
				longit = tokens[4];
			}
			output << wpt << ", " << country << ", " << lat << ", " << longit << endl;
			//DD = d + (min/60) + (sec/3600)
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

