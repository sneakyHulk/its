#include <pylon/BaslerUniversalInstantCamera.h>
#include <pylon/GrabResultPtr.h>
#include <pylon/ImageEventHandler.h>
#include <pylon/PylonIncludes.h>

#include <fstream>
#include <opencv2/opencv.hpp>
#include <ranges>
#include <thread>
#include <vector>

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
	std::ofstream out("/result/image.raw", std::ios::binary);
	out.write(static_cast<const char*>(ptrGrabResult->GetBuffer()), static_cast<int>(ptrGrabResult->GetBufferSize()));

	out.close();
}

void save_png(Pylon::CGrabResultPtr const& ptrGrabResult) {
	cv::Mat bayer_image(static_cast<int>(ptrGrabResult->GetHeight()), static_cast<int>(ptrGrabResult->GetWidth()), CV_8UC1, ptrGrabResult->GetBuffer());
	cv::Mat image;
	cv::cvtColor(bayer_image, image, cv::COLOR_BayerRG2BGR);

	cv::imwrite("/result/image.png", image);
}

int main(int argc, char* argv[]) {
	// Before using any pylon methods, the pylon runtime must be initialized.
	PylonRAII pylon_raii;

	try {
		while (true) {
			// try to get camera in controller mode -> if device is controlled by another application it will fail -> controller_mode is set to false
			bool controller_mode = true;

			Pylon::CDeviceInfo info;
			info.SetDeviceClass(Pylon::BaslerGigEDeviceClass);
			Pylon::CBaslerUniversalInstantCamera camera;
			try {
				camera.Attach(Pylon::CTlFactory::GetInstance().CreateFirstDevice(info));
				camera.Open();
			} catch (const Pylon::GenericException& e) {
				std::string error_description = e.GetDescription();

				// if other error than "The device is controlled by another application."
				if (error_description.find("The device is controlled by another application.") == std::string::npos) {
					common::println("[Camera]: ", e.GetDescription());

					return 1;
				}

				controller_mode = false;
			}

			if (controller_mode) {
				// Set transmission type to "multicast"
				camera.GetStreamGrabberParams().TransmissionType = Basler_UniversalStreamParams::TransmissionType_Multicast;
				// camera.GetStreamGrabberParams().DestinationAddr = "239.0.0.1";    // These are default values.
				// camera.GetStreamGrabberParams().DestinationPort = 49152;

				camera.PixelFormat.SetValue(Basler_UniversalCameraParams::PixelFormatEnums::PixelFormat_BayerRG8);
			} else {
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

			common::println("[Camera]: Camera in ", (controller_mode ? "controller" : "monitor"), " mode.");

			try {
				camera.StartGrabbing();
				Pylon::CGrabResultPtr ptrGrabResult;

				int frames = 0;
				while (camera.IsGrabbing()) {
					camera.RetrieveResult(1000, ptrGrabResult, Pylon::TimeoutHandling_ThrowException);

					cv::Size size(static_cast<int>(ptrGrabResult->GetWidth()), static_cast<int>(ptrGrabResult->GetHeight()));
					cv::Mat bayer_image(size, CV_8UC1, ptrGrabResult->GetBuffer());
					cv::Mat image;
					cv::cvtColor(bayer_image, image, cv::COLOR_BayerRG2BGR);

					if (++frames > (controller_mode ? 100 : 150)) {
						break;
					}
				}
			} catch (Pylon::TimeoutException const& e) {
				if (!controller_mode) {
					common::println("[Camera]: Application in controller mode terminated! Switching to controller mode...");
					continue;
				}
			}

			common::println("[Camera]: Cannot access camera anymore! Reconnect in 5s...");
			std::this_thread::sleep_for(5s);
		}
	} catch (std::exception const& e) {
		common::println("[Camera]: ", e.what());
	} catch (...) {
		common::println("[Camera]: Unknown error occurred!");
	}

	// camera.RetrieveResult(1000, ptrGrabResult, Pylon::TimeoutHandling_ThrowException);
	//
	// cv::VideoWriter video;
	// video.open(controller_mode ? "/result/video_controller.mp4" : "/result/video_monitor.mp4", cv::VideoWriter::fourcc('m', 'p', '4', 'v'), 15., size);
	//
	// while (camera.IsGrabbing()) {
	//	video.write(image);
	//	if (++frames > (controller_mode ? 1000 : 999)) {
	//		break;
	//	}
	//}
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