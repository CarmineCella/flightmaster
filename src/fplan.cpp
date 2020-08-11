// fplan.cpp
// 

#include <stdexcept>
#include <iostream>

#include "fplan.h"

using namespace std;

#define VERSION 0.1

int main (int argc, char* argv[]) {
	cout << BOLDYELLOW << "[fplan, ver. " << VERSION << "]" << endl << endl;
	cout << RESET << "(c) 2020, www.carminecella.com" << endl << endl;

	try {
		if (argc < 2) {
			throw runtime_error ("syntax is 'fplan command [args]\n\n" \
								"where 'command' can be:\n\n" \
								"update................................ update/download aeronautical databases\n"\
								"[metars|tafs|winds|info] station(s)... fetch specified information for station(s)\n"\
								"flog config.txt output.txt............ generate a flight log\n\n" \
								"examples:\n\n"\
								"fplan update\n"\
								"fplan metars KOAK\n"\
								"fplan tafs KLVK\n"\
								"fplan winds SFO LAS\n"\
								"fplan info LIPY LIDF LIPB\n"\
								"fplan flog my_flight.txt flightlog.txt\n\n");
		}
		std::string command = argv[1];

		if (command == "update") {
			update_dbs (cout);
		} else if (command == "metars" || command == "tafs" || command == "winds" || command == "info") {
			if (argc < 3) {
				throw runtime_error ("station(s) missing");
			}
			if (command == "metars" || command == "tafs") {
				for (unsigned i = 2; i < argc; ++i) {
					std::vector<std::string> res = get_metar_taf (argv[i], command);
					for (unsigned i = 0; i < res.size (); ++i) {
						cout << res.at (i) << endl;
					}
				}
				cout << endl;
			} else if (command == "winds") {
				for (unsigned i = 2; i < argc; ++i) {
					cout << get_winds_aloft (argv[i]) << endl;
				}
				cout << endl;
			} else if (command == "info") {
				CSV_data airports, frequencies, runways, navaids;
				load_dbs (cout, airports, frequencies, runways, navaids);

				for (unsigned i = 2; i < argc; ++i) {
					Airport a1 = get_airport_info (argv[i], airports, frequencies, runways);
					cout << a1.info << endl;
					for (unsigned i = 0; i < a1.runways.size (); ++i) {
						cout << a1.runways.at (i) << endl;
					}
					for (unsigned i = 0; i < a1.frequencies.size (); ++i) {
						cout << a1.frequencies.at (i) << endl;
					}
					cout << endl;       
				}
			}
		} else if (command == "flog") {
			if (argc != 4) {
				throw runtime_error ("required syntax is 'flog input.txt output.txt");
			}
		} else {
			throw runtime_error ("invalid command specified");
		}

		Parameters p;

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

