#include <pylon/BaslerUniversalInstantCameraArray.h>
#include <pylon/GrabResultPtr.h>
#include <pylon/PylonIncludes.h>

#include <boost/circular_buffer.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>
#include <vector>

#include "BaslerCameraBase.h"
#include "ImageDataRaw.h"
#include "common_exception.h"
#include "Pusher.h"

class BaslerCameraNode : public Pusher<ImageDataRaw>, public BaslerCameraBase {
	Pylon::CBaslerUniversalInstantCamera _camera;

	std::string _camera_name;

   public:
	BaslerCameraNode(std::string&& camera_name, std::string&& mac_address) : _camera_name(std::forward<decltype(camera_name)>(camera_name)) {
		Pylon::DeviceInfoList_t device_list;
		Pylon::CTlFactory::GetInstance().EnumerateDevices(device_list);

		if (device_list.empty()) throw common::Exception("[BaslerCamerasNode]: No Basler camera devices found!");

		try {
			for (auto& device : device_list) {
				if (std::string device_mac_address = device.GetMacAddress().c_str(); device_mac_address != mac_address) {
					common::println("[BaslerCamerasNode]: Found unknown basler device with model name '", device.GetModelName(), "', ip address '", device.GetIpAddress(), "', and mac address '", device.GetMacAddress(), "'.");
					continue;
				}

				common::println("[BaslerCamerasNode]: Found device ", _camera_name, " with model name '", device.GetModelName(), "', ip address '", device.GetIpAddress(), "', and mac address '", device.GetMacAddress(), "'.");
				std::this_thread::sleep_for(1s);

				_camera.Attach(Pylon::CTlFactory::GetInstance().CreateDevice(device));
				_camera.Open();

				// Enabling PTP Clock Synchronization
				enable_ptp(_camera, _camera_name, std::chrono::milliseconds(10));

				_camera.AcquisitionFrameRateEnable.SetValue(true);
				_camera.AcquisitionFrameRateAbs.SetValue(50.0);
			}

			common::print("[BaslerCamerasNode]: Starting grabbing...");

			_camera.StartGrabbing(Pylon::EGrabStrategy::GrabStrategy_LatestImageOnly);

			common::println("done!");
		} catch (Pylon::GenericException const& e) {
			common::println("[BaslerCamerasNode]: Error: ", e.GetDescription());

			std::rethrow_exception(std::current_exception());
		}
	}

   private:
	ImageDataRaw push() final {
		do {
			try {
				Pylon::CGrabResultPtr ptrGrabResult;
				_camera.RetrieveResult(5000, ptrGrabResult, Pylon::TimeoutHandling_ThrowException);

				if (!ptrGrabResult->GrabSucceeded()) {
					common::println("[BaslerCameraNode ", _camera_name, "]: ", ptrGrabResult->GetErrorDescription());

					continue;
				}

				ImageDataRaw data;
				data.timestamp = ptrGrabResult->GetTimeStamp();
				data.source = _camera_name;
				data.image_raw = std::vector<std::uint8_t>(static_cast<const std::uint8_t*>(ptrGrabResult->GetBuffer()), static_cast<const std::uint8_t*>(ptrGrabResult->GetBuffer()) + ptrGrabResult->GetBufferSize());

				std::chrono::nanoseconds image_creation_timestamp = std::chrono::nanoseconds(ptrGrabResult->GetTimeStamp());
				std::chrono::nanoseconds current_server_timestamp = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now()).time_since_epoch();

				static boost::circular_buffer<std::chrono::nanoseconds> fps_buffers = boost::circular_buffer<std::chrono::nanoseconds>(20, std::chrono::nanoseconds(0));
				auto fps = 20. / std::chrono::duration<double>(current_server_timestamp - fps_buffers.front()).count();
				fps_buffers.push_back(current_server_timestamp);

				common::println("[BaslerCameraNode]: ", _camera_name, " grab successful at ", image_creation_timestamp, " with grab duration ", current_server_timestamp - image_creation_timestamp, " and fps ", fps, ".");

				return data;

			} catch (Pylon::TimeoutException const& e) {
				common::println("[BaslerCameraNode]: ", e.GetDescription());
			}
		} while (true);
	}
};