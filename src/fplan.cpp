// fplan.cpp
// 

#include <stdexcept>
#include <iostream>

#include "fplan.h"

using namespace std;

int main (int argc, char* argv[]) {
	try {
		cout << "load dbs..."; cout.flush();
		CSV_data airports, frequencies, runways;
		load_dbs (airports, frequencies, runways);
		cout << "done" << endl;
		Parameters p;
		p.metars =  get_metar_taf ("KOAK", "metars");
		for (unsigned i = 0; i < p.metars.size (); ++i) {
			std::cout << p.metars.at (i) << std::endl;
		}

		string w =  get_winds_aloft ("SFO");
		cout << w << endl;
		int dir, speed;
		get_wind_info (w, 24000, dir, speed);
		cout << "wind: " << dir << ", " << speed << endl;

		cout << "wind correction: 305@100Kts -> " << compute_wind_correction (dir, speed, 100, 305) << endl;

		Fix f;

		f.navaids = get_navaids ("KLVK");
		for (unsigned i = 0; i < 	f.navaids.size (); ++i) {
			std::cout <<	f.navaids.at (i) << std::endl;
		}		

		get_airport_info ("KOAK", airports, frequencies, runways);

		get_airport_info ("KO88", airports, frequencies, runways);
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

