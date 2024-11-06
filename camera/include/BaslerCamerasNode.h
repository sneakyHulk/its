#pragma once

#include <pylon/BaslerUniversalInstantCameraArray.h>
#include <pylon/GrabResultPtr.h>
#include <pylon/PylonIncludes.h>

#include <boost/circular_buffer.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>
#include <csignal>
#include <vector>

#include "ImageDataRaw.h"
#include "common_exception.h"
#include "node.h"

class BaslerCamerasNode : public Pusher<ImageDataRaw> {
   public:
	struct MacAddressConfig {
		std::string address;
	};

	struct CameraNameMacAddressIndexConfig {
		struct CameraNameTag {};
		struct MacAddressTag {};
		struct IndexTag {};
		std::string camera_name;
		std::string mac_address;
		int index;
	};

	explicit BaslerCamerasNode(std::map<std::string, MacAddressConfig>&& camera_name_mac_address) {
		auto index = 0;
		for (auto const& [cam_name, mac_address] : camera_name_mac_address) {
			_camera_name_mac_address_index_map.emplace(cam_name, mac_address.address, index++);
		}
		_cameras.Initialize(index);

		Pylon::CDeviceInfo info;
		info.SetDeviceClass(Pylon::BaslerGigEDeviceClass);

		Pylon::DeviceInfoList_t device_list;
		Pylon::CTlFactory::GetInstance().EnumerateDevices(device_list);
		if (device_list.empty()) {
			throw common::Exception("[BaslerCamerasNode]: No Basler camera devices found!");
		}
		try {
			for (auto& mac_address_indexing = _camera_name_mac_address_index_map.get<CameraNameMacAddressIndexConfig::MacAddressTag>(); auto& device : device_list) {
				auto const& config = mac_address_indexing.find(device.GetMacAddress().c_str());

				if (config == mac_address_indexing.end()) {
					common::println("[BaslerCamerasNode]: Found unknown basler device with model name '", device.GetModelName(), "', ip address '", device.GetIpAddress(), "', and mac address '", device.GetMacAddress(), "'.");
					continue;
				}

				common::println("[BaslerCamerasNode]: Found device ", config->camera_name, " with model name '", device.GetModelName(), "', ip address '", device.GetIpAddress(), "', and mac address '", device.GetMacAddress(), "'.");
				std::this_thread::sleep_for(1s);

				_cameras[config->index].Attach(Pylon::CTlFactory::GetInstance().CreateDevice(device));

				_cameras[config->index].Open();
				// Enabling PTP Clock Synchronization
				if (_cameras[config->index].GevIEEE1588.GetValue()) {
					common::println("[BaslerCamerasNode]: ", config->camera_name, " IEEE1588 already enabled!");

					_cameras[config->index].GevIEEE1588DataSetLatch.Execute();
					while (_cameras[config->index].GevIEEE1588StatusLatched() == Basler_UniversalCameraParams::GevIEEE1588StatusLatchedEnums::GevIEEE1588StatusLatched_Initializing) {
						std::this_thread::sleep_for(1ms);
					}

					auto current_offset = std::chrono::nanoseconds(std::abs(_cameras[config->index].GevIEEE1588OffsetFromMaster()));

					common::println("[BaslerCamerasNode]: ", config->camera_name, " Offset from master approx. ", current_offset, ".");
				} else {
					common::println("[BaslerCamerasNode]: ", config->camera_name, " Enable PTP clock synchronization...");
					_cameras[config->index].GevIEEE1588.SetValue(true);

					// Wait until all PTP network devices are sufficiently synchronized. https://docs.baslerweb.com/precision-time-protocol#checking-the-status-of-the-ptp-clock-synchronization
					common::println("[BaslerCamerasNode]: ", config->camera_name, " Waiting for PTP network devices to be sufficiently synchronized...");
					boost::circular_buffer<std::chrono::nanoseconds> clock_offsets(10, std::chrono::nanoseconds::max());
					do {
						_cameras[config->index].GevIEEE1588DataSetLatch.Execute();

						while (_cameras[config->index].GevIEEE1588StatusLatched() == Basler_UniversalCameraParams::GevIEEE1588StatusLatchedEnums::GevIEEE1588StatusLatched_Initializing) {
							std::this_thread::sleep_for(1ms);
						}

						auto current_offset = std::chrono::nanoseconds(std::abs(_cameras[config->index].GevIEEE1588OffsetFromMaster()));
						clock_offsets.push_back(current_offset);
						common::println("[BaslerCamerasNode]: ", config->camera_name, " Offset from master approx. ", current_offset, ", max offset is ", *std::max_element(clock_offsets.begin(), clock_offsets.end()), ".");

						std::this_thread::sleep_for(1s);
					} while (*std::max_element(clock_offsets.begin(), clock_offsets.end()) > 15ms);

					common::println("[BaslerCamerasNode]: ", config->camera_name, " Highest offset from master <= 15ms. Can start to grab images.");
				}
			}

			common::print("[BaslerCamerasNode]: Starting grabbing...");

			_cameras.StartGrabbing(Pylon::EGrabStrategy::GrabStrategy_LatestImageOnly);

			common::println("done!");
		} catch (Pylon::GenericException const& e) {
			common::println("[BaslerCamerasNode]: Error: ", e.GetDescription());

			std::rethrow_exception(std::current_exception());
		}
	}

   private:
	Pylon::CBaslerUniversalInstantCameraArray _cameras;
	boost::multi_index_container<CameraNameMacAddressIndexConfig, boost::multi_index::indexed_by<boost::multi_index::ordered_unique<boost::multi_index::tag<BaslerCamerasNode::CameraNameMacAddressIndexConfig::CameraNameTag>,
	                                                                                                 boost::multi_index::member<CameraNameMacAddressIndexConfig, std::string, &CameraNameMacAddressIndexConfig::camera_name>>,
	                                                                  boost::multi_index::ordered_unique<boost::multi_index::tag<BaslerCamerasNode::CameraNameMacAddressIndexConfig::MacAddressTag>,
	                                                                      boost::multi_index::member<CameraNameMacAddressIndexConfig, std::string, &CameraNameMacAddressIndexConfig::mac_address>>,
	                                                                  boost::multi_index::ordered_unique<boost::multi_index::tag<BaslerCamerasNode::CameraNameMacAddressIndexConfig::IndexTag>,
	                                                                      boost::multi_index::member<CameraNameMacAddressIndexConfig, int, &CameraNameMacAddressIndexConfig::index>>>>
	    _camera_name_mac_address_index_map;

	ImageDataRaw push() final {
		static boost::circular_buffer<std::chrono::nanoseconds> fps_buffer(20, std::chrono::nanoseconds(0));

		auto& index_indexing = _camera_name_mac_address_index_map.get<CameraNameMacAddressIndexConfig::IndexTag>();
		do {
			try {
				Pylon::CGrabResultPtr ptrGrabResult;
				_cameras.RetrieveResult(5000, ptrGrabResult, Pylon::TimeoutHandling_ThrowException);

				auto const& config = index_indexing.find(ptrGrabResult->GetCameraContext());

				if (!ptrGrabResult->GrabSucceeded()) {
					common::println("[BaslerCamerasNode ", config->camera_name, "]: ", ptrGrabResult->GetErrorDescription());

					continue;
				}

				// Timestamp offset from master:
				//_cameras[config->index].GevIEEE1588DataSetLatch.Execute();
				// while (_cameras[config->index].GevIEEE1588StatusLatched() == Basler_UniversalCameraParams::GevIEEE1588StatusLatchedEnums::GevIEEE1588StatusLatched_Initializing)
				//	;
				// common::println("[BaslerCamerasNode]: offset in timestamp from master: ", std::chrono::nanoseconds(std::abs(_cameras[config->index].GevIEEE1588OffsetFromMaster())));

				// Timestamp offset from server:
				// _cameras[config->index].GevTimestampControlLatch.Execute();
				// std::chrono::nanoseconds current_server_timestamp = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now()).time_since_epoch();
				// std::chrono::nanoseconds current_camera_timestamp = std::chrono::nanoseconds(_cameras[config->index].GevTimestampValue.GetValue());
				// common::println("[BaslerCamerasNode]: Timestamp offset: ", current_server_timestamp < current_camera_timestamp ? current_camera_timestamp - current_server_timestamp : current_server_timestamp - current_camera_timestamp);

				common::println("[BaslerCamerasNode]: ", config->camera_name, " grab successful.");
				ImageDataRaw data;
				data.timestamp = ptrGrabResult->GetTimeStamp();
				data.source = config->camera_name;
				data.image_raw = std::vector<std::uint8_t>(static_cast<const std::uint8_t*>(ptrGrabResult->GetBuffer()), static_cast<const std::uint8_t*>(ptrGrabResult->GetBuffer()) + ptrGrabResult->GetBufferSize());
				std::chrono::nanoseconds current_server_timestamp = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now()).time_since_epoch();
				auto fps = 20. / (current_server_timestamp - fps_buffer.front()).count();
				fps_buffer.push_back(current_server_timestamp);
				std::chrono::nanoseconds current_camera_timestamp = std::chrono::nanoseconds(data.timestamp);
				common::println("[BaslerCamerasNode]: Grab duration: ", current_server_timestamp < current_camera_timestamp ? current_camera_timestamp - current_server_timestamp : current_server_timestamp - current_camera_timestamp,
				    ", with fps: ", fps, ".");

				return data;

			} catch (Pylon::TimeoutException const& e) {
				common::println("[BaslerCamerasNode]: ", e.GetDescription());

				continue;
			}
		} while (true);
	}
};