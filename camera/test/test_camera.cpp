#include <pylon/PylonIncludes.h>

#include <ranges>
#include <vector>

#include "common_exception.h"
#include "common_output.h"

int main() {
	Pylon::PylonInitialize();

	{
		Pylon::DeviceInfoList device_list;
		Pylon::CTlFactory::GetInstance().EnumerateDevices(device_list);
		if (device_list.empty()) throw common::Exception("No Basler devices found!");

		Pylon::CInstantCameraArray cameras(device_list.size());
		for (auto i = 0; i < device_list.size(); ++i) {
			common::println("Found device with model name '", device_list.at(i).GetModelName(), "', ip address '", device_list.at(i).GetIpAddress(), "', and mac address '", device_list.at(i).GetMacAddress(), "'.");
			cameras[i].Attach(Pylon::CTlFactory::GetInstance().CreateDevice(device_list.at(i)));
		}

		cameras.StartGrabbing();

		while (cameras.IsGrabbing()) {
			Pylon::CGrabResultPtr result;
			bool grabbed = cameras.RetrieveResult(1000, result);

			common::println("width: ", result->GetWidth(), ", height:", result->GetHeight());

			if (!grabbed) {
				common::println("No image retrieved!");
			}
		}
	}

	Pylon::PylonTerminate();
}