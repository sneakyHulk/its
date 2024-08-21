#include "camera_node.h"

#include <boost/circular_buffer.hpp>

Camera::Camera() { init_camera(); }
ImageData Camera::input_function() {
	do {
		if (!camera.IsGrabbing()) {
			common::println("[Camera]: Image Grabbing failed! Reconnect in 5s...");

			std::this_thread::sleep_for(5s);
			init_camera();

			continue;
		}

		Pylon::CGrabResultPtr ptrGrabResult;

		camera.RetrieveResult(1000, ptrGrabResult, Pylon::TimeoutHandling_ThrowException);

		if (!ptrGrabResult->GrabSucceeded()) {
			if (!controller_mode) {
				common::println("[Camera]: Application in controller mode terminated! Switching to controller mode...");
			} else {
				common::println("[Camera]: Image Grabbing failed! Reconnect in 5s...");

				std::this_thread::sleep_for(5s);
			}

			init_camera();

			continue;
		}

		cv::Size size(static_cast<int>(ptrGrabResult->GetWidth()), static_cast<int>(ptrGrabResult->GetHeight()));
		cv::Mat bayer_image(size, CV_8UC1, ptrGrabResult->GetBuffer());
		cv::Mat image;
		cv::cvtColor(bayer_image, image, cv::COLOR_BayerRG2BGR);

		return ImageData{image, 1, static_cast<int>(ptrGrabResult->GetWidth()), static_cast<int>(ptrGrabResult->GetHeight()), " "};
	} while (true);
}
void Camera::init_camera() {
	Pylon::CDeviceInfo info;
	info.SetDeviceClass(Pylon::BaslerGigEDeviceClass);

	while (true) {
		try {
			camera.Attach(Pylon::CTlFactory::GetInstance().CreateFirstDevice(info));

			controller_mode = is_camera_in_controller_mode(camera);

			if (controller_mode) {
				common::println("[Camera]: Camera in controller mode.");

				// Set transmission type to "multicast"
				camera.GetStreamGrabberParams().TransmissionType = Basler_UniversalStreamParams::TransmissionType_Multicast;
				// camera.GetStreamGrabberParams().DestinationAddr = "239.0.0.1";    // These are default values.
				// camera.GetStreamGrabberParams().DestinationPort = 49152;

				camera.PixelFormat.SetValue(Basler_UniversalCameraParams::PixelFormatEnums::PixelFormat_BayerRG8);

				// Enabling PTP Clock Synchronization
				if (camera.GevIEEE1588.GetValue()) {
					common::println("[Camera]: IEEE1588 enabled, disabling...");
					camera.GevIEEE1588.SetValue(false);
					std::this_thread::sleep_for(5s);
				}
				common::println("[Camera]: Enable PTP clock synchronization...");
				camera.GevIEEE1588.SetValue(true);

				// Wait until all PTP network devices are sufficiently synchronized. https://docs.baslerweb.com/precision-time-protocol#checking-the-status-of-the-ptp-clock-synchronization
				common::println("[Camera]: Waiting for PTP network devices to be sufficiently synchronized...");
				boost::circular_buffer<std::chrono::nanoseconds> clock_offsets(10, std::chrono::nanoseconds::max());
				do {
					camera.GevIEEE1588DataSetLatch();

					if (camera.GevIEEE1588StatusLatched() == Basler_UniversalCameraParams::GevIEEE1588StatusLatchedEnums::GevIEEE1588StatusLatched_Initializing) {
						continue;
					}

					auto current_offset = std::chrono::nanoseconds(std::abs(camera.GevIEEE1588OffsetFromMaster()));
					clock_offsets.push_back(current_offset);

					common::println("[Camera]: Offset from master approx. ", current_offset, ", max offset is ", *std::max_element(clock_offsets.begin(), clock_offsets.end()));
					std::this_thread::sleep_for(100ms);
				} while (*std::max_element(clock_offsets.begin(), clock_offsets.end()) > 1ms);

				// common::println("[Camera]: Waiting for slave mode...");
				// while (camera.GevIEEE1588Status() != Basler_UniversalCameraParams::GevIEEE1588StatusEnums::GevIEEE1588Status_Slave);

				common::println("[Camera]: Highest offset from master < 1ms. Can start to grab images.");
			} else {
				common::println("[Camera]: Camera in monitor mode.");

				std::this_thread::sleep_for(20s);

				// The default configuration must be removed when monitor mode is selected
				// because the monitoring application is not allowed to modify any parameter settings.
				camera.RegisterConfiguration((Pylon::CConfigurationEventHandler*)NULL, Pylon::RegistrationMode_ReplaceAll, Pylon::Cleanup_None);

				// Set MonitorModeActive to true to act as monitor
				camera.MonitorModeActive = true;

				camera.Open();

				// Select transmission type. If the camera is already controlled by another application
				// and configured for multicast, the active camera configuration can be used
				// (IP Address and Port will be set automatically).
				camera.GetStreamGrabberParams().TransmissionType = Basler_UniversalStreamParams::TransmissionType_UseCameraConfig;
			}

			camera.StartGrabbing();

			break;
		} catch (Pylon::GenericException const& e) {
			common::println("[Camera]: ", e.GetDescription(), "! Reconnect in 5s...");

			camera.Close();

			std::this_thread::sleep_for(5s);
			continue;
		} catch (...) {
			common::println("[Camera]: Unknown error occurred!");
			throw;
		}
	}
}
bool Camera::is_camera_in_controller_mode(Pylon::CBaslerUniversalInstantCamera& camera) {
	try {
		camera.Open();

		return true;
	} catch (Pylon::GenericException const& e) {
		std::string error_description = e.GetDescription();

		// if other error than "The device is controlled by another application."
		if (error_description.find("The device is controlled by another application.") == std::string::npos) {
			throw common::Exception("[Camera]: ", e.GetDescription());
		}

		return false;
	}
}
