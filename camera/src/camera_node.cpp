#include "camera_node.h"

#include <boost/circular_buffer.hpp>
#include <csignal>
#include <fstream>
#include <utility>

class PylonRAII {
	static void pylon_uninitialization_handler(int signal) {
		common::println("pylon_uninitialization_handler()");
		Pylon::PylonTerminate();

		std::_Exit(signal);
	}

   public:
	PylonRAII() {
		common::println("PylonRAII()");
		std::signal(SIGTERM, pylon_uninitialization_handler);
		std::signal(SIGABRT, pylon_uninitialization_handler);
		std::signal(SIGINT, pylon_uninitialization_handler);
		Pylon::PylonInitialize();
	}
	~PylonRAII() {
		common::println("~PylonRAII()");
		Pylon::PylonTerminate();
	}
};

Pylon::CGrabResultPtr BaslerCameras::input_function() {
	do {
		try {
			Pylon::CGrabResultPtr ptrGrabResult;
			cameras.RetrieveResult(5000, ptrGrabResult, Pylon::TimeoutHandling_ThrowException);

			if (!ptrGrabResult->GrabSucceeded()) {
				common::println("[BaslerCameras ", index_to_cam_name[ptrGrabResult->GetCameraContext()], "]: ", ptrGrabResult->GetErrorDescription());

				continue;
			}

			common::println("[BaslerCameras]: ", index_to_cam_name[ptrGrabResult->GetCameraContext()], " grab successful.");
			return ptrGrabResult;

		} catch (Pylon::TimeoutException const& e) {
			common::println("[BaslerCameras]: ", e.GetDescription());

			continue;
		}
	} while (true);
}

void BaslerCameras::init_cameras() {
	Pylon::CDeviceInfo info;
	info.SetDeviceClass(Pylon::BaslerGigEDeviceClass);

	Pylon::DeviceInfoList device_list;
	Pylon::CTlFactory::GetInstance().EnumerateDevices(device_list);
	if (device_list.empty()) {
		throw common::Exception("No Basler devices found!");
	}

	cameras.Initialize(device_list.size());
	index_to_cam_name.resize(device_list.size());

	try {
		for (auto i = 0; Pylon::CDeviceInfo & device : device_list) {
			index_to_cam_name[i] = mac_to_cam_name.at(device.GetMacAddress().c_str());
			common::println("[BaslerCameras]: Found device ", index_to_cam_name[i], " with model name '", device.GetModelName(), "', ip address '", device.GetIpAddress(), "', and mac address '", device.GetMacAddress(), "'.");
			cameras[i].Attach(Pylon::CTlFactory::GetInstance().CreateDevice(device));
			std::this_thread::sleep_for(1s);
			++i;

			// Enabling PTP Clock Synchronization
			if (cameras[i].GevIEEE1588.GetValue()) {
				common::println("[BaslerCameras]: ", index_to_cam_name[i], " IEEE1588 already enabled!");
			} else {
				common::println("[BaslerCameras]: ", index_to_cam_name[i], " Enable PTP clock synchronization...");
				cameras[i].GevIEEE1588.SetValue(true);

				// Wait until all PTP network devices are sufficiently synchronized. https://docs.baslerweb.com/precision-time-protocol#checking-the-status-of-the-ptp-clock-synchronization
				common::println("[BaslerCameras]: ", index_to_cam_name[i], " Waiting for PTP network devices to be sufficiently synchronized...");
				boost::circular_buffer<std::chrono::nanoseconds> clock_offsets(10, std::chrono::nanoseconds::max());
				do {
					cameras[i].GevIEEE1588DataSetLatch();

					if (cameras[i].GevIEEE1588StatusLatched() == Basler_UniversalCameraParams::GevIEEE1588StatusLatchedEnums::GevIEEE1588StatusLatched_Initializing) {
						continue;
					}

					auto current_offset = std::chrono::nanoseconds(std::abs(cameras[i].GevIEEE1588OffsetFromMaster()));
					clock_offsets.push_back(current_offset);
					common::println("[BaslerCameras]: ", index_to_cam_name[i], " Offset from master approx. ", current_offset, ", max offset is ", *std::max_element(clock_offsets.begin(), clock_offsets.end()));

					std::this_thread::sleep_for(1s);
				} while (*std::max_element(clock_offsets.begin(), clock_offsets.end()) > 15ms);

				common::println("[BaslerCameras]: ", index_to_cam_name[i], " Highest offset from master < 15ms. Can start to grab images.");
			}
		}

		cameras.StartGrabbing();

		common::println("[BaslerCameras]: Started grabbing!");
	} catch (Pylon::GenericException const& e) {
		common::println("[BaslerCameras]: ", e.GetDescription());

		throw std::current_exception();
	}
}

BaslerCameras::BaslerCameras(std::map<std::string, std::string> const& mac_and_cam_names) : mac_to_cam_name(mac_and_cam_names) {
	static PylonRAII pylon_initialization;

	init_cameras();
}
BaslerSaveRAW::BaslerSaveRAW(BaslerCameras const& cameras, std::filesystem::path folder) : cameras(cameras), folder(std::move(folder)) {
	for (auto const& e : cameras.index_to_cam_name) {
		if (!std::filesystem::create_directories(folder / e)) {
			throw common::Exception("[BaslerSaveRAW]: Create directory ", folder / e, " failed!");
		}

		common::println("[BaslerSaveRAW]: Created directory ", folder / e, ".");
	}
}
void BaslerSaveRAW::output_function(Pylon::CGrabResultPtr const& data) {
	std::ofstream raw_image(folder / cameras.index_to_cam_name[data->GetCameraContext()] /
	                        (std::to_string(std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now()).time_since_epoch().count()) + '_' + std::to_string(data->GetTimeStamp())));

	raw_image.write(static_cast<const char*>(data->GetBuffer()), data->GetBufferSize());
}
