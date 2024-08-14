#include <pylon/PylonIncludes.h>

#include <ranges>
#include <vector>

#include "common_exception.h"
#include "common_output.h"

int main() {
	Pylon::PylonInitialize();

	auto& pylon_instance = Pylon::CTlFactory::GetInstance();

	Pylon::DeviceInfoList device_list;
	pylon_instance.EnumerateDevices(device_list);
	if (device_list.empty()) throw common::Exception("No Basler devices found!");


	common::println("Using device ", device_list.at(0).GetModelName());
	Pylon::CInstantCamera camera(Pylon::CTlFactory::GetInstance().CreateFirstDevice());
	common::println("Using device ", camera.GetDeviceInfo().GetModelName());


	// Pylon::CInstantCameraArray cameras(device_list.size());
	//
	// for (auto i = 0; i < device_list.size(); ++i) {
	//	common::println("Found device with model name '", device_list.at(i).GetModelName(), "', ip address '", device_list.at(i).GetIpAddress(), "', and mac address '", device_list.at(i).GetMacAddress(), "'.");
	//
	//	cameras[i].Attach(pylon_instance.CreateDevice(device_list.at(i)));
	//}

	Pylon::PylonTerminate();
	return 0;
}