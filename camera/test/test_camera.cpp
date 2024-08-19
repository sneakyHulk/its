#include <pylon/BaslerUniversalInstantCamera.h>
#include <pylon/GrabResultPtr.h>
#include <pylon/ImageEventHandler.h>
#include <pylon/PylonIncludes.h>

#include <ranges>
#include <vector>

#include "common_exception.h"
#include "common_output.h"

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

int main(int argc, char* argv[]) {
	// Before using any pylon methods, the pylon runtime must be initialized.
	Pylon::PylonInitialize();

	try {
		Pylon::CDeviceInfo info;
		info.SetDeviceClass(Pylon::BaslerGigEDeviceClass);

		Pylon::CBaslerUniversalInstantCamera camera(Pylon::CTlFactory::GetInstance().CreateFirstDevice());
		camera.Open();
		camera.GetStreamGrabberParams().TransmissionType = Basler_UniversalStreamParams::TransmissionType_Multicast;
		camera.StartGrabbing();

		Pylon::CGrabResultPtr ptrGrabResult;

		while (camera.IsGrabbing())  {
			camera.RetrieveResult( 5000, ptrGrabResult, Pylon::TimeoutHandling_ThrowException );
		}

	} catch (const Pylon::GenericException& e) {
		common::println("An exception occurred: ", e.GetDescription());

		return 1;
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