#include <pylon/BaslerUniversalInstantCamera.h>
#include <pylon/BaslerUniversalInstantCameraArray.h>
#include <pylon/GrabResultPtr.h>
#include <pylon/ImageEventHandler.h>
#include <pylon/PylonIncludes.h>

#include <array>
#include <chrono>
#include <exception>
#include <thread>

using namespace std::chrono_literals;

#define num_cams 4

int main() {
	Pylon::PylonInitialize();

	Pylon::DeviceInfoList device_list;
	Pylon::CTlFactory::GetInstance().EnumerateDevices(device_list);
	if (device_list.empty()) throw std::logic_error("No Basler devices found!");
	//if (device_list.size() < num_cams) throw std::logic_error("Not enough Basler devices found!");

	Pylon::CBaslerUniversalInstantCameraArray cameras(num_cams);
	std::array<std::string, num_cams> camera_names;

	for (auto i = 0; i < device_list.size(); ++i) {
		camera_names[i] = device_list.at(i).GetModelName() + " @ " + device_list.at(i).GetIpAddress() + " # " + device_list.at(i).GetMacAddress();
		std::cout << "Found " << i << " device with model name '" << device_list.at(i).GetModelName() << "', ip address '" << device_list.at(i).GetIpAddress() << "', and mac address '" << device_list.at(i).GetMacAddress() << "'."
		          << std::endl;

		while (true) {
			try {
				cameras[i].Attach(Pylon::CTlFactory::GetInstance().CreateDevice(device_list.at(i)));
				cameras[i].Open();

				// Set transmission type to "multicast"
				cameras[i].GetStreamGrabberParams().TransmissionType = Basler_UniversalStreamParams::TransmissionType_Multicast;
				cameras[i].GetStreamGrabberParams().DestinationAddr = (std::string("239.255.0.") + std::to_string(++i)).c_str();
				cameras[i].GetStreamGrabberParams().DestinationPort = 49152 + i;

				cameras[i].PixelFormat.SetValue(Basler_UniversalCameraParams::PixelFormatEnums::PixelFormat_BayerRG8);

				break;
			} catch (Pylon::GenericException const& e) {
				std::cout << "[Camera " << camera_names[i] << "]: " << e.GetDescription() << "! Reconnect in 5s..." << std::endl;

				std::this_thread::sleep_for(5s);
				continue;
			}
		}
	}

	cameras.StartGrabbing();

	Pylon::PylonTerminate();
}