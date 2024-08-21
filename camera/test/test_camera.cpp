#include <pylon/BaslerUniversalInstantCamera.h>
#include <pylon/GrabResultPtr.h>
#include <pylon/ImageEventHandler.h>
#include <pylon/PylonIncludes.h>

#include <boost/circular_buffer.hpp>
#include <filesystem>
#include <fstream>
#include <opencv2/opencv.hpp>
#include <ranges>
#include <thread>

#include "common_exception.h"
#include "common_output.h"

using namespace std::chrono_literals;

class PylonRAII {
   public:
	PylonRAII() { Pylon::PylonInitialize(); }
	~PylonRAII() { Pylon::PylonTerminate(); }
};

class CConfigurationEventPrinter : public Pylon::CConfigurationEventHandler {
   public:
	void OnAttach(Pylon::CInstantCamera& /*camera*/) final { std::cout << "OnAttach event" << std::endl; }
	void OnAttached(Pylon::CInstantCamera& camera) final { std::cout << "OnAttached event for device " << camera.GetDeviceInfo().GetModelName() << std::endl; }
	void OnOpen(Pylon::CInstantCamera& camera) final { std::cout << "OnOpen event for device " << camera.GetDeviceInfo().GetModelName() << std::endl; }
	void OnOpened(Pylon::CInstantCamera& camera) final { std::cout << "OnOpened event for device " << camera.GetDeviceInfo().GetModelName() << std::endl; }
	void OnGrabStart(Pylon::CInstantCamera& camera) final { std::cout << "OnGrabStart event for device " << camera.GetDeviceInfo().GetModelName() << std::endl; }
	void OnGrabStarted(Pylon::CInstantCamera& camera) final { std::cout << "OnGrabStarted event for device " << camera.GetDeviceInfo().GetModelName() << std::endl; }
	void OnGrabStop(Pylon::CInstantCamera& camera) final { std::cout << "OnGrabStop event for device " << camera.GetDeviceInfo().GetModelName() << std::endl; }
	void OnGrabStopped(Pylon::CInstantCamera& camera) final { std::cout << "OnGrabStopped event for device " << camera.GetDeviceInfo().GetModelName() << std::endl; }
	void OnClose(Pylon::CInstantCamera& camera) final { std::cout << "OnClose event for device " << camera.GetDeviceInfo().GetModelName() << std::endl; }
	void OnClosed(Pylon::CInstantCamera& camera) final { std::cout << "OnClosed event for device " << camera.GetDeviceInfo().GetModelName() << std::endl; }
	void OnDestroy(Pylon::CInstantCamera& camera) final { std::cout << "OnDestroy event for device " << camera.GetDeviceInfo().GetModelName() << std::endl; }
	void OnDestroyed(Pylon::CInstantCamera& /*camera*/) final { std::cout << "OnDestroyed event" << std::endl; }
	void OnDetach(Pylon::CInstantCamera& camera) final { std::cout << "OnDetach event for device " << camera.GetDeviceInfo().GetModelName() << std::endl; }
	void OnDetached(Pylon::CInstantCamera& camera) final { std::cout << "OnDetached event for device " << camera.GetDeviceInfo().GetModelName() << std::endl; }
	void OnGrabError(Pylon::CInstantCamera& camera, const char* errorMessage) final {
		std::cout << "OnGrabError event for device " << camera.GetDeviceInfo().GetModelName() << std::endl;
		std::cout << "Error Message: " << errorMessage << std::endl;
	}
	void OnCameraDeviceRemoved(Pylon::CInstantCamera& camera) final { std::cout << "OnCameraDeviceRemoved event for device " << camera.GetDeviceInfo().GetModelName() << std::endl; }
};

class CImageEventPrinter : public Pylon::CImageEventHandler {
   public:
	void OnImagesSkipped(Pylon::CInstantCamera& camera, size_t countOfSkippedImages) final {
		std::cout << "OnImagesSkipped event for device " << camera.GetDeviceInfo().GetModelName() << std::endl;
		std::cout << countOfSkippedImages << " images have been skipped." << std::endl;
		std::cout << std::endl;
	}

	void OnImageGrabbed(Pylon::CInstantCamera& camera, const Pylon::CGrabResultPtr& ptrGrabResult) final {
		std::cout << "OnImageGrabbed event for device " << camera.GetDeviceInfo().GetModelName() << std::endl;

		// Image grabbed successfully?
		if (ptrGrabResult->GrabSucceeded()) {
			std::cout << "SizeX: " << ptrGrabResult->GetWidth() << std::endl;
			std::cout << "SizeY: " << ptrGrabResult->GetHeight() << std::endl;
			const uint8_t* pImageBuffer = (uint8_t*)ptrGrabResult->GetBuffer();
			std::cout << "Gray value of first pixel: " << (uint32_t)pImageBuffer[0] << std::endl;
			std::cout << std::endl;
		} else {
			std::cout << "Error: " << std::hex << ptrGrabResult->GetErrorCode() << std::dec << " " << ptrGrabResult->GetErrorDescription() << std::endl;
		}
	}
};

void save_raw(Pylon::CGrabResultPtr const& ptrGrabResult) {
	std::ofstream out(std::filesystem::path(CMAKE_SOURCE_DIR) / std::filesystem::path("result/image.raw"), std::ios::binary);
	out.write(static_cast<const char*>(ptrGrabResult->GetBuffer()), static_cast<int>(ptrGrabResult->GetBufferSize()));

	out.close();
}

void save_png(Pylon::CGrabResultPtr const& ptrGrabResult) {
	cv::Mat bayer_image(static_cast<int>(ptrGrabResult->GetHeight()), static_cast<int>(ptrGrabResult->GetWidth()), CV_8UC1, ptrGrabResult->GetBuffer());
	cv::Mat image;
	cv::cvtColor(bayer_image, image, cv::COLOR_BayerBG2BGR);

	cv::imwrite(std::filesystem::path(CMAKE_SOURCE_DIR) / std::filesystem::path("result/image.png"), image);
}

int main(int argc, char* argv[]) {
	// Before using any pylon methods, the pylon runtime must be initialized.
	PylonRAII pylon_raii;

	Pylon::CDeviceInfo info;
	info.SetDeviceClass(Pylon::BaslerGigEDeviceClass);

	while (true) {
		Pylon::CBaslerUniversalInstantCamera camera;
		bool controller_mode;

		try {
			controller_mode = true;

			camera.Attach(Pylon::CTlFactory::GetInstance().CreateFirstDevice(info));
			camera.Open();  // can fail

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

				std::this_thread::sleep_for(1s);
			} while (*std::max_element(clock_offsets.begin(), clock_offsets.end()) > 1ms);

			// common::println("[Camera]: Waiting for slave mode...");
			// while (camera.GevIEEE1588Status() != Basler_UniversalCameraParams::GevIEEE1588StatusEnums::GevIEEE1588Status_Slave);

			common::println("[Camera]: Highest offset from master < 1ms. Can start to grab images.");

			common::println("[Camera]: Start to grab images...");
			camera.StartGrabbing();
		} catch (Pylon::GenericException const& e) {
			std::string error_description = e.GetDescription();

			// errors related to camera already being in controller mode
			if (error_description.find("The device is controlled by another application.") != std::string::npos || error_description.find("Node is not writable!") != std::string::npos) {
				common::println("[Camera]: Camera already in controller mode. Switching to monitor mode...");

				controller_mode = false;
			} else {
				common::println("[Camera]: ", e.GetDescription(), "! Reconnect in 5s...");

				std::this_thread::sleep_for(5s);
				continue;
			}

			try {
				camera.Attach(Pylon::CTlFactory::GetInstance().CreateFirstDevice(info));

				// The default configuration must be removed when monitor mode is selected
				// because the monitoring application is not allowed to modify any parameter settings.
				camera.RegisterConfiguration((Pylon::CConfigurationEventHandler*)NULL, Pylon::RegistrationMode_ReplaceAll, Pylon::Cleanup_None);

				// Set MonitorModeActive to true to act as monitor
				camera.MonitorModeActive = true;

				camera.Open();

				common::println("[Camera]: Camera in monitor mode.");

				// Select transmission type. If the camera is already controlled by another application
				// and configured for multicast, the active camera configuration can be used
				// (IP Address and Port will be set automatically).
				camera.GetStreamGrabberParams().TransmissionType = Basler_UniversalStreamParams::TransmissionType_UseCameraConfig;

				camera.StartGrabbing();
			} catch (Pylon::GenericException const& e) {
				common::println("[Camera]: ", e.GetDescription(), "! Reconnect in 5s...");

				std::this_thread::sleep_for(5s);
				continue;
			}
		} catch (...) {
			common::println("[Camera]: Unknown error occurred!");
			throw;
		}

		int frame = 0;
		do {
			if (!camera.IsGrabbing()) {
				common::println("[Camera]: Image Grabbing failed! Reconnect in 5s...");

				std::this_thread::sleep_for(5s);
				break;
			}

			Pylon::CGrabResultPtr ptrGrabResult;
			try {
				camera.RetrieveResult(1000, ptrGrabResult, Pylon::TimeoutHandling_ThrowException);
			} catch (Pylon::TimeoutException const& e) {
				if (!controller_mode) {
					common::println("[Camera]: Application in controller mode terminated! Switching to controller mode...");
				} else {
					common::println("[Camera]: Image Grabbing failed! Reconnect in 5s...");

					std::this_thread::sleep_for(5s);
				}

				break;
			}

			if (!ptrGrabResult->GrabSucceeded()) {
				if (!controller_mode) {
					common::println("[Camera]: Application in controller mode terminated! Switching to controller mode...");
				} else {
					common::println("[Camera]: Image Grabbing failed! Reconnect in 5s...");

					std::this_thread::sleep_for(5s);
				}

				break;
			}

			save_png(ptrGrabResult);
			save_raw(ptrGrabResult);

			if (++frame > (controller_mode ? 500 : 1000)) break;
		} while (true);
	}
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
