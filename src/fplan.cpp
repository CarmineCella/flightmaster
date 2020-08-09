// fplan.cpp
// 

#include <stdexcept>
#include <iostream>

#include "fplan.h"

using namespace std;

int main (int argc, char* argv[]) {
	try {

		Parameters p;
		p.metars =  get_metar_taf ("KOAK", "tafs");
		for (unsigned i = 0; i < p.metars.size (); ++i) {
			std::cout << p.metars.at (i) << std::endl;
		}
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

