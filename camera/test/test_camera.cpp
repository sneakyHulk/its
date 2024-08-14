#include <pylon/PylonIncludes.h>

#include "common_output.h"

int main() {
	Pylon::PylonInitialize();

	auto& pylon_instance = Pylon::CTlFactory::GetInstance();

	Pylon::DeviceInfoList device_list;
	pylon_instance.EnumerateDevices(device_list);

	if (device_list.empty()) common::println("No Basler devices found!");

	for (auto const& e : device_list) {
		common::println(e.GetModelName(), ": ", e.GetIpAddress(), ", ", e.GetMacAddress());
	}

	Pylon::PylonTerminate();
	return 0;
}