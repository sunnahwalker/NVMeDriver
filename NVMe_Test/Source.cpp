////////////////////////////////////////////////////////////////////////////
//
//		Ahsan Uddin
//		© Sunnahwalker Productions 2017
//		Windows NVMe App to check Drive information
//		App is based on Microsft WIndows Windows NVMe Drivers
//		Version 1.01
//		3/12/2017
//		For questions nor concerns please conatct me at ahsan89@gmail.com
//
/////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <windows.h>

//Windows DDK specific includes
#include <ntddscsi.h>

//Local libraries
#include "NVMEDriver.h"

using namespace std;

int main() {

	BOOL status = FALSE;

	cout << "Welcome to SunnahWalker's NVMe App!" << endl;
	cout << __DATE__ << " " << __TIME__ << endl << endl;

	status = find_port();

	if (status == TRUE) {
		status = Do_IDFY();
	}

	if (status == TRUE) {
		status = Do_SMART();
	}

	if (status == TRUE) {
		status = Close_Handle();
	}

	return ((status == TRUE) ? EXIT_SUCCESS : EXIT_FAILURE);
}