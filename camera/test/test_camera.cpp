#include <pylon/PylonIncludes.h>

#include <ranges>
#include <vector>

#include "common_exception.h"
#include "common_output.h"

int main() {
	Pylon::PylonInitialize();

	{
		// Create an instant camera object with the camera device found first.
		Pylon::CInstantCamera camera(Pylon::CTlFactory::GetInstance().CreateFirstDevice());

		// Print the model name of the camera.
		std::cout << "Using device " << camera.GetDeviceInfo().GetModelName() << std::endl;

		// Start the grabbing of c_countOfImagesToGrab images.
		// The camera device is parameterized with a default configuration which
		// sets up free-running continuous acquisition.
		camera.StartGrabbing(1);

		// This smart pointer will receive the grab result data.
		Pylon::CGrabResultPtr ptrGrabResult;

		// Camera.StopGrabbing() is called automatically by the RetrieveResult() method
		// when c_countOfImagesToGrab images have been retrieved.
		while (camera.IsGrabbing()) {
			// Wait for an image and then retrieve it. A timeout of 5000 ms is used.
			camera.RetrieveResult(5000, ptrGrabResult, Pylon::TimeoutHandling_ThrowException);

			// Image grabbed successfully?
			if (ptrGrabResult->GrabSucceeded()) {
				// Access the image data.
				std::cout << "SizeX: " << ptrGrabResult->GetWidth() << std::endl;
				std::cout << "SizeY: " << ptrGrabResult->GetHeight() << std::endl;
				const uint8_t* pImageBuffer = (uint8_t*)ptrGrabResult->GetBuffer();
				std::cout << "Gray value of first pixel: " << (uint32_t)pImageBuffer[0] << std::endl << std::endl;

			} else {
				std::cout << "Error: " << std::hex << ptrGrabResult->GetErrorCode() << std::dec << " " << ptrGrabResult->GetErrorDescription() << std::endl;
			}
		}
	}

	//{
	//	Pylon::DeviceInfoList device_list;
	//	Pylon::CTlFactory::GetInstance().EnumerateDevices(device_list);
	//	if (device_list.empty()) throw common::Exception("No Basler devices found!");
	//
	//	Pylon::CInstantCameraArray cameras(device_list.size());
	//	for (auto i = 0; i < device_list.size(); ++i) {
	//		common::println("Found device with model name '", device_list.at(i).GetModelName(), "', ip address '", device_list.at(i).GetIpAddress(), "', and mac address '", device_list.at(i).GetMacAddress(), "'.");
	//		cameras[i].Attach(Pylon::CTlFactory::GetInstance().CreateDevice(device_list.at(i)));
	//	}
	//
	//	cameras.StartGrabbing(Pylon::EGrabStrategy::GrabStrategy_LatestImages);
	//
	//	while (cameras.IsGrabbing()) {
	//		Pylon::CGrabResultPtr result;
	//
	//		common::println("Retrieving Images...");
	//		bool grabbed = cameras[0].RetrieveResult(1000, result);
	//
	//		if (!grabbed) {
	//			common::println("No image retrieved!");
	//			continue;
	//		}
	//
	//		common::println("width: ", result->GetWidth(), ", height:", result->GetHeight());
	//	}
	//}

	Pylon::PylonTerminate();
}