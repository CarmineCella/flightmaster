// fplan.cpp
// 

#include <stdexcept>
#include <iostream>

using namespace std;

int main (int argc, char* argv[]) {
	try {

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

