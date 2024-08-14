#include <pylon/PylonIncludes.h>

#include <ranges>
#include <vector>

#include "common_exception.h"
#include "common_output.h"

// Grab.cpp
/*
    Note: Before getting started, Basler recommends reading the "Programmer's Guide" topic
    in the pylon C++ API documentation delivered with pylon.
    If you are upgrading to a higher major version of pylon, Basler also
    strongly recommends reading the "Migrating from Previous Versions" topic in the pylon C++ API documentation.

    This sample illustrates how to grab and process images using the CInstantCamera class.
    The images are grabbed and processed asynchronously, i.e.,
    while the application is processing a buffer, the acquisition of the next buffer is done
    in parallel.

    The CInstantCamera class uses a pool of buffers to retrieve image data
    from the camera device. Once a buffer is filled and ready,
    the buffer can be retrieved from the camera object for processing. The buffer
    and additional image data are collected in a grab result. The grab result is
    held by a smart pointer after retrieval. The buffer is automatically reused
    when explicitly released or when the smart pointer object is destroyed.
*/

// Include files to use the pylon API.
#include <pylon/PylonIncludes.h>
#ifdef PYLON_WIN_BUILD
#include <pylon/PylonGUI.h>
#endif

// Namespace for using pylon objects.
using namespace Pylon;

// Namespace for using cout.
using namespace std;

// Number of images to be grabbed.
static const uint32_t c_countOfImagesToGrab = 100;

int main(int /*argc*/, char* /*argv*/[]) {
	// The exit code of the sample application.
	int exitCode = 0;

	// Before using any pylon methods, the pylon runtime must be initialized.
	PylonInitialize();

	try {
		// Create an instant camera object with the camera device found first.
		CInstantCamera camera(CTlFactory::GetInstance().CreateFirstDevice());

		// Print the model name of the camera.
		cout << "Using device " << camera.GetDeviceInfo().GetModelName() << endl;

		// The parameter MaxNumBuffer can be used to control the count of buffers
		// allocated for grabbing. The default value of this parameter is 10.
		camera.MaxNumBuffer = 5;

		// Start the grabbing of c_countOfImagesToGrab images.
		// The camera device is parameterized with a default configuration which
		// sets up free-running continuous acquisition.
		camera.StartGrabbing(c_countOfImagesToGrab);

		// This smart pointer will receive the grab result data.
		CGrabResultPtr ptrGrabResult;

		// Camera.StopGrabbing() is called automatically by the RetrieveResult() method
		// when c_countOfImagesToGrab images have been retrieved.
		while (camera.IsGrabbing()) {
			// Wait for an image and then retrieve it. A timeout of 5000 ms is used.
			camera.RetrieveResult(5000, ptrGrabResult, TimeoutHandling_ThrowException);

			// Image grabbed successfully?
			if (ptrGrabResult->GrabSucceeded()) {
				// Access the image data.
				cout << "SizeX: " << ptrGrabResult->GetWidth() << endl;
				cout << "SizeY: " << ptrGrabResult->GetHeight() << endl;
				const uint8_t* pImageBuffer = (uint8_t*)ptrGrabResult->GetBuffer();
				cout << "Gray value of first pixel: " << (uint32_t)pImageBuffer[0] << endl << endl;

#ifdef PYLON_WIN_BUILD
				// Display the grabbed image.
				Pylon::DisplayImage(1, ptrGrabResult);
#endif
			} else {
				cout << "Error: " << std::hex << ptrGrabResult->GetErrorCode() << std::dec << " " << ptrGrabResult->GetErrorDescription() << endl;
			}
		}
	} catch (const GenericException& e) {
		// Error handling.
		cerr << "An exception occurred." << endl << e.GetDescription() << endl;
		exitCode = 1;
	}

	// Comment the following two lines to disable waiting on exit.
	cerr << endl << "Press enter to exit." << endl;
	while (cin.get() != '\n')
		;

	// Releases all pylon resources.
	PylonTerminate();

	return exitCode;
}

// int main() {
//	Pylon::PylonInitialize();
//
//	{
//		Pylon::DeviceInfoList device_list;
//		Pylon::CTlFactory::GetInstance().EnumerateDevices(device_list);
//		if (device_list.empty()) throw common::Exception("No Basler devices found!");
//
//		Pylon::CInstantCameraArray cameras(device_list.size());
//		for (auto i = 0; i < device_list.size(); ++i) {
//			common::println("Found device with model name '", device_list.at(i).GetModelName(), "', ip address '", device_list.at(i).GetIpAddress(), "', and mac address '", device_list.at(i).GetMacAddress(), "'.");
//			cameras[i].Attach(Pylon::CTlFactory::GetInstance().CreateDevice(device_list.at(i)));
//		}
//
//		cameras.StartGrabbing(Pylon::EGrabStrategy::GrabStrategy_LatestImages);
//
//		while (cameras.IsGrabbing()) {
//			Pylon::CGrabResultPtr result;
//
//			common::println("Retrieving Images...");
//			bool grabbed = cameras[0].RetrieveResult(1000, result);
//
//			if (!grabbed) {
//				common::println("No image retrieved!");
//				continue;
//			}
//
//			common::println("width: ", result->GetWidth(), ", height:", result->GetHeight());
//		}
//	}
//
//	Pylon::PylonTerminate();
// }