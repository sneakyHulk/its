#include "BaslerCameraNode.h"

#include <pylon/GrabResultPtr.h>
#include <pylon/PylonIncludes.h>

#include <boost/circular_buffer.hpp>

#include "common_output.h"

template <bool v2>
BaslerCameraNode<v2>::BaslerCameraNode(std::string&& camera_name, std::string&& mac_address) : _camera_name(std::forward<decltype(camera_name)>(camera_name)) {
	Pylon::DeviceInfoList_t device_list;
	Pylon::CTlFactory::GetInstance().EnumerateDevices(device_list);

	if (device_list.empty()) common::println_critical_loc("No Basler camera devices found!");

	try {
		for (auto& device : device_list) {
			if (std::string device_mac_address = device.GetMacAddress().c_str(); device_mac_address != mac_address) {
				common::println_warn_loc("Found unknown basler device with model name '", device.GetModelName(), "', ip address '", device.GetIpAddress(), "', and mac address '", device.GetMacAddress(), "'.");
				continue;
			}

			common::println_loc("Found device ", _camera_name, " with model name '", device.GetModelName(), "', ip address '", device.GetIpAddress(), "', and mac address '", device.GetMacAddress(), "'.");
			std::this_thread::sleep_for(1s);

			_camera.Attach(Pylon::CTlFactory::GetInstance().CreateDevice(device));
			_camera.Open();

			// Enabling PTP Clock Synchronization
			BaslerCameraBase<v2>::enable_ptp(_camera, _camera_name, std::chrono::milliseconds(10));
		}

		common::print_loc("Starting grabbing...");

		_camera.StartGrabbing(Pylon::EGrabStrategy::GrabStrategy_LatestImageOnly);

		common::println("done!");
	} catch (Pylon::GenericException const& e) {
		common::println_critical_loc(e.GetDescription());
	}
}

template <bool v2>
BaslerCameraNode<v2>::BaslerCameraNode(std::string&& camera_name, std::string&& mac_address, double const fps) : BaslerCameraNode(std::forward<decltype(camera_name)>(camera_name), std::forward<decltype(mac_address)>(mac_address)) {
	try {
		_camera.AcquisitionFrameRateEnable.SetValue(true);
		_camera.AcquisitionFrameRateAbs.SetValue(fps);
	} catch (Pylon::GenericException const& e) {
		common::println_critical_loc(e.GetDescription());
	}
}

template <bool v2>
ImageDataRaw BaslerCameraNode<v2>::push() {
	do {
		try {
			Pylon::CGrabResultPtr ptrGrabResult;
			_camera.RetrieveResult(5000, ptrGrabResult, Pylon::TimeoutHandling_ThrowException);

			if (!ptrGrabResult->GrabSucceeded()) {
				common::println_error_loc(_camera_name, ": ", ptrGrabResult->GetErrorDescription());

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

			common::println_debug_loc(_camera_name, ": grab successful at ", image_creation_timestamp, " with grab duration ", current_server_timestamp - image_creation_timestamp, " and fps ", fps, ".");

			return data;

		} catch (Pylon::TimeoutException const& e) {
			common::println_error_loc(e.GetDescription());
		}
	} while (true);
}

template class BaslerCameraNode<true>;
template class BaslerCameraNode<false>;