// flightmaster.cpp
// 

#include <stdexcept>
#include <iostream>

#include "utilities.h"
#include "flightmaster.h"

using namespace std;

#define VERSION 0.5

// TODO: custom winds (183,23), FIX.txt, custom waypoints.csv, weight & balance

int main (int argc, char* argv[]) {
	cout << BOLDYELLOW << "[flightmaster, ver. " << VERSION << "]" << endl << endl;
	cout << RESET << "(c) 2020, www.carminecella.com" << endl << endl;

	try {
		if (argc < 2) {
			throw runtime_error (RESET "syntax is 'flightmaster command [args]\n\n" \
								"where 'command' can be:\n\n" \
								"update................................ update/download aeronautical databases\n"\
								"[metars|tafs|winds|info] station(s)... fetch specified information for station(s)\n"\
								"flog config.txt output.txt............ generate a flight log\n" \
								"xwind hdg wind_dir wind_speed......... compute the wind components on a heading\n\n"\
								"wcorr hdg speed wind_dir wind_speed... compute the wind correction on a heading\n\n"\
								"examples:\n\n"\
								"flightmaster update\n"\
								"flightmaster metars KOAK LIPY LIPE\n"\
								"flightmaster tafs KLVK\n"\
								"flightmaster winds SFO LAS\n"\
								"flightmaster info LIDF\n"\
								"flightmaster flog my_flight.txt flightlog.txt\n"\
								"flightmaster xwind 06 100 8\n"\
								"flightmaster rwcorr 270 100 100 10\n");
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
					if (a1.info.size ()) {
						cout << a1.info << endl;
						for (unsigned i = 0; i < a1.runways.size (); ++i) {
							cout << a1.runways.at (i) << endl;
						}
						for (unsigned i = 0; i < a1.frequencies.size (); ++i) {
							cout << a1.frequencies.at (i) << endl;
						}
						cout << endl;       
					} else {
						std::vector<std::string> navaids_info = get_navaid_info (argv[i], navaids);
						if (navaids_info.size () == 0) {
							throw runtime_error ("cannot find specified station");
						}

						for (unsigned i = 0; i < navaids_info.size (); ++i) {
							cout << navaids_info.at (i) << endl;
						}
						cout << endl;
					} 
				}
			}
		} else if (command == "flog") {
			if (argc != 4) {
				throw runtime_error ("required syntax is 'flog input.txt output.txt");
			}

			Parameters p;
			load_dbs (cout, p.airports_db, p.frequencies_db, p.runways_db, p.navaids_db);
			cout << "configuring..." << std::endl; cout.flush ();
			read_config (argv[2], p, cout);
			cout << endl;

			std::string flog = compile_flight (p, cout);
			cout << endl;

			ofstream out (argv[3]);
			if (!out.good ()) {
				throw runtime_error ("cannot create output file");
			}
			out << flog << std::endl;
		} else if (command == "xwind") {
			if (argc != 5) {
				throw runtime_error ("required syntax is 'xwind heading wind_dir wind_speed");
			}			
			int hdg = atol (argv[2]);
			if (hdg < 0 || hdg > 359) throw runtime_error ("invalid heading");

			int wind_dir = atol (argv[3]);
			if (wind_dir < 0 || wind_dir > 359) throw runtime_error ("invalid wind direction");

			double wind_speed = atof (argv[4]);
			if (wind_speed < 0) throw runtime_error ("invalid wind speed");

			double xwind_speed = 0;
			double tailwind_speed = 0;
			compute_xwind (hdg, wind_dir, wind_speed, xwind_speed, tailwind_speed);

			cout << "heading "<< hdg << ": ";
			if (xwind_speed < 0) cout <<  "left ";
			else cout << "right ";
			cout << "xwind speed = " << fabs (xwind_speed) << ", ";
			if (tailwind_speed > 0) cout << "headwind ";
			else cout << "tailwind ";
			cout << "speed = " << fabs (tailwind_speed) << endl;

		} else if (command == "wcorr") {
			if (argc != 6) {
				throw runtime_error ("required syntax is 'wcorr heading speed wind_dir wind_speed");
			}			
			int hdg = atol (argv[2]);
			if (hdg < 0 || hdg > 359) throw runtime_error ("invalid heading");

			double speed = atof (argv[3]);
			if (speed < 0) throw runtime_error ("invalid speed");

			int wind_dir = atol (argv[4]);
			if (wind_dir < 0 || wind_dir > 359) throw runtime_error ("invalid wind direction");

			double wind_speed = atof (argv[5]);
			if (wind_speed < 0) throw runtime_error ("invalid wind speed");

			double ground_speed = 0;
			int new_head = compute_wind_correction (wind_dir, wind_speed, speed, hdg, ground_speed);

			cout << "heading "<< hdg << " -> " << new_head << " (gs = " << ground_speed << ")" << endl;
		}else {
			throw runtime_error ("invalid command specified");
		}
	}
	catch (exception& e) {
		cout << RED <<  "error: " << e.what () << RESET << endl << endl;
	}
	catch (...) {
		cout << RED << "fatal error: unknown exception" << RESET << endl << endl;
	}
	return 0;
}

// EOF

