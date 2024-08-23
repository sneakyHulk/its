#include "camera_node.h"

#include <boost/circular_buffer.hpp>
#include <filesystem>
#include <utility>

ImageData Camera::input_function() {
	do {
		if (!camera.IsGrabbing()) {
			common::println("[Camera ", cam_name, "]: Image Grabbing failed! Reconnect in 5s...");

			std::this_thread::sleep_for(5s);

			init_camera();
			continue;
		}

		Pylon::CGrabResultPtr ptrGrabResult;
		try {
			camera.RetrieveResult(5000, ptrGrabResult, Pylon::TimeoutHandling_ThrowException);
		} catch (Pylon::TimeoutException const& e) {
			if (!controller_mode) {
				common::println("[Camera ", cam_name, "]: Application in controller mode terminated! Switching to controller mode...");
			} else {
				common::println("[Camera ", cam_name, "]: Image Grabbing failed! Reconnect in 5s...");

				std::this_thread::sleep_for(5s);
			}

			init_camera();
			continue;
		}

		if (!ptrGrabResult->GrabSucceeded()) {
			if (!controller_mode) {
				common::println("[Camera ", cam_name, "]: Application in controller mode terminated! Switching to controller mode...");
			} else {
				common::println("[Camera ", cam_name, "]: Image Grabbing failed! Reconnect in 5s...");

				std::this_thread::sleep_for(5s);
			}

			init_camera();
			continue;
		}

		auto width = static_cast<int>(ptrGrabResult->GetWidth());
		auto height = static_cast<int>(ptrGrabResult->GetHeight());
		std::uint64_t timestamp = ptrGrabResult->GetTimeStamp();

		cv::Mat bayer_image(height, width, CV_8UC1, ptrGrabResult->GetBuffer());
		cv::Mat image;
		cv::cvtColor(bayer_image, image, cv::COLOR_BayerBG2BGR);

		return ImageData(image, timestamp, width, height, cam_name);

	} while (true);
}

void Camera::save_png() {
	Pylon::CGrabResultPtr ptrGrabResult;
	camera.RetrieveResult(5000, ptrGrabResult, Pylon::TimeoutHandling_ThrowException);

	auto width = static_cast<int>(ptrGrabResult->GetWidth());
	auto height = static_cast<int>(ptrGrabResult->GetHeight());
	std::uint64_t timestamp = ptrGrabResult->GetTimeStamp();

	cv::Mat bayer_image(height, width, CV_8UC1, ptrGrabResult->GetBuffer());
	cv::Mat image;
	cv::cvtColor(bayer_image, image, cv::COLOR_BayerBG2BGR);

	cv::imwrite(std::filesystem::path(CMAKE_SOURCE_DIR) / std::filesystem::path(std::string("result/image_") + cam_name + "_" + std::to_string(timestamp) + ".png"), image);
}

void Camera::init_camera() {
	Pylon::CDeviceInfo info;
	info.SetDeviceClass(Pylon::BaslerGigEDeviceClass);

	Pylon::DeviceInfoList device_list;
	Pylon::CTlFactory::GetInstance().EnumerateDevices(device_list);
	if (device_list.empty()) throw common::Exception("No Basler devices found!");

	for (Pylon::CDeviceInfo& device : device_list) {
		common::println("[Camera ", cam_name, "]: Found device with model name '", device.GetModelName(), "', ip address '", device.GetIpAddress(), "', and mac address '", device.GetMacAddress(), "'.");
		if (std::string(device.GetMacAddress()) == mac_adress) {
			common::println("[Camera ", cam_name, "]: Found ", cam_name, " via mac address match.");

			while (true) {
				try {
					controller_mode = true;

					camera.Attach(Pylon::CTlFactory::GetInstance().CreateDevice(device));
					camera.Open();  // can fail

					common::println("[Camera ", cam_name, "]: Camera in controller mode.");

					// Set transmission type to "multicast"
					camera.GetStreamGrabberParams().TransmissionType = Basler_UniversalStreamParams::TransmissionType_Multicast;
					camera.GetStreamGrabberParams().DestinationAddr = (std::string("239.0.0.") + std::to_string(++ip_index)).c_str();  // These are default values.
					camera.GetStreamGrabberParams().DestinationPort = 49152 + ip_index;

					camera.PixelFormat.SetValue(Basler_UniversalCameraParams::PixelFormatEnums::PixelFormat_BayerRG8);

					// Enabling PTP Clock Synchronization
					if (camera.GevIEEE1588.GetValue()) {
						common::println("[Camera ", cam_name, "]: IEEE1588 already enabled!");
					} else {
						common::println("[Camera ", cam_name, "]: Enable PTP clock synchronization...");
						camera.GevIEEE1588.SetValue(true);
					}

					// Wait until all PTP network devices are sufficiently synchronized. https://docs.baslerweb.com/precision-time-protocol#checking-the-status-of-the-ptp-clock-synchronization
					common::println("[Camera ", cam_name, "]: Waiting for PTP network devices to be sufficiently synchronized...");
					boost::circular_buffer<std::chrono::nanoseconds> clock_offsets(10, std::chrono::nanoseconds::max());
					do {
						camera.GevIEEE1588DataSetLatch();

						if (camera.GevIEEE1588StatusLatched() == Basler_UniversalCameraParams::GevIEEE1588StatusLatchedEnums::GevIEEE1588StatusLatched_Initializing) {
							continue;
						}

						auto current_offset = std::chrono::nanoseconds(std::abs(camera.GevIEEE1588OffsetFromMaster()));
						clock_offsets.push_back(current_offset);
						common::println("[Camera ", cam_name, "]: Offset from master approx. ", current_offset, ", max offset is ", *std::max_element(clock_offsets.begin(), clock_offsets.end()));

						std::this_thread::sleep_for(1s);
					} while (*std::max_element(clock_offsets.begin(), clock_offsets.end()) > 15ms);

					// common::println("[Camera]: Waiting for slave mode...");
					// while (camera.GevIEEE1588Status() != Basler_UniversalCameraParams::GevIEEE1588StatusEnums::GevIEEE1588Status_Slave);

					common::println("[Camera ", cam_name, "]: Highest offset from master < 15ms. Can start to grab images.");

					common::println("[Camera ", cam_name, "]: Start to grab images...");
					camera.StartGrabbing();

					save_png();

					break;
				} catch (Pylon::GenericException const& e) {
					std::string error_description = e.GetDescription();

					// errors related to camera already being in controller mode
					if (error_description.find("The device is controlled by another application.") != std::string::npos) {
						common::println("[Camera ", cam_name, "]: Camera already in controller mode. Switching to monitor mode...");

						controller_mode = false;
					} else if (error_description.find("Node is not writable") != std::string::npos) {
						common::println("[Camera ", cam_name, "]: Camera already in controller mode. Switching to monitor mode...");

						controller_mode = false;
					} else {
						common::println("[Camera ", cam_name, "]: ", e.GetDescription(), "! Reconnect in 5s...");

						std::this_thread::sleep_for(5s);
						continue;
					}

					try {
						camera.Attach(Pylon::CTlFactory::GetInstance().CreateDevice(device));

						// The default configuration must be removed when monitor mode is selected
						// because the monitoring application is not allowed to modify any parameter settings.
						camera.RegisterConfiguration((Pylon::CConfigurationEventHandler*)NULL, Pylon::RegistrationMode_ReplaceAll, Pylon::Cleanup_None);

						// Set MonitorModeActive to true to act as monitor
						camera.MonitorModeActive = true;

						camera.Open();

						common::println("[Camera ", cam_name, "]: Camera in monitor mode.");

						// Select transmission type. If the camera is already controlled by another application
						// and configured for multicast, the active camera configuration can be used
						// (IP Address and Port will be set automatically).
						camera.GetStreamGrabberParams().TransmissionType = Basler_UniversalStreamParams::TransmissionType_UseCameraConfig;

						common::println("[Camera ", cam_name, "]: Start to grab images...");
						camera.StartGrabbing();

						break;
					} catch (Pylon::GenericException const& e) {
						common::println("[Camera ", cam_name, "]: ", e.GetDescription(), "! Reconnect in 5s...");

						std::this_thread::sleep_for(5s);
						continue;
					}
				} catch (...) {
					common::println("[Camera ", cam_name, "]: Unknown error occurred!");
					throw;
				}
			}

			return;
		}
	}

	throw common::Exception("[Camera ", cam_name, "]: Couldn't find ", cam_name, " via mac address match.");
}
Camera::Camera(std::string mac_address, std::string cam_name) : mac_adress(std::move(mac_address)), cam_name(std::move(cam_name)) { init_camera(); }

int Camera::ip_index = 1;