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

class BaslerCamerasNode : public InputNode<ImageDataRaw> {
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

		Pylon::DeviceInfoList device_list;
		Pylon::CTlFactory::GetInstance().EnumerateDevices(device_list);
		if (device_list.empty()) {
			throw common::Exception("[BaslerCamerasNode]: No Basler camera devices found!");
		}

		try {
			for (auto& mac_address_indexing = _camera_name_mac_address_index_map.get<CameraNameMacAddressIndexConfig::MacAddressTag>(); Pylon::CDeviceInfo & device : device_list) {
				auto const& config = *mac_address_indexing.find(device.GetMacAddress().c_str());

				_cameras[config.index].Attach(Pylon::CTlFactory::GetInstance().CreateDevice(device));
				common::println("[BaslerCamerasNode]: Found device ", config.camera_name, " with model name '", device.GetModelName(), "', ip address '", device.GetIpAddress(), "', and mac address '", device.GetMacAddress(), "'.");
				std::this_thread::sleep_for(1s);

				// Enabling PTP Clock Synchronization
				if (_cameras[config.index].GevIEEE1588.GetValue()) {
					common::println("[BaslerCamerasNode]: ", config.camera_name, " IEEE1588 already enabled!");
				} else {
					common::println("[BaslerCamerasNode]: ", config.camera_name, " Enable PTP clock synchronization...");
					_cameras[config.index].GevIEEE1588.SetValue(true);

					// Wait until all PTP network devices are sufficiently synchronized. https://docs.baslerweb.com/precision-time-protocol#checking-the-status-of-the-ptp-clock-synchronization
					common::println("[BaslerCamerasNode]: ", config.camera_name, " Waiting for PTP network devices to be sufficiently synchronized...");
					boost::circular_buffer<std::chrono::nanoseconds> clock_offsets(10, std::chrono::nanoseconds::max());
					do {
						_cameras[config.index].GevIEEE1588DataSetLatch();

						if (_cameras[config.index].GevIEEE1588StatusLatched() == Basler_UniversalCameraParams::GevIEEE1588StatusLatchedEnums::GevIEEE1588StatusLatched_Initializing) {
							std::this_thread::sleep_for(1ms);
							continue;
						}

						auto current_offset = std::chrono::nanoseconds(std::abs(_cameras[config.index].GevIEEE1588OffsetFromMaster()));
						clock_offsets.push_back(current_offset);
						common::println("[BaslerCamerasNode]: ", config.camera_name, " Offset from master approx. ", current_offset, ", max offset is ", *std::max_element(clock_offsets.begin(), clock_offsets.end()));

						std::this_thread::sleep_for(1s);
					} while (*std::max_element(clock_offsets.begin(), clock_offsets.end()) > 15ms);

					common::println("[BaslerCamerasNode]: ", config.camera_name, " Highest offset from master <= 15ms. Can start to grab images.");
				}
			}

			common::println("[BaslerCamerasNode]: Starting grabbing...");

			_cameras.StartGrabbing();

			common::println("[BaslerCamerasNode]: Started grabbing.");
		} catch (Pylon::GenericException const& e) {
			common::println("[BaslerCamerasNode]: ", e.GetDescription());

			throw std::current_exception();
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

	ImageDataRaw input_function() final {
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

				common::println("[BaslerCamerasNode]: ", config->camera_name, " grab successful.");
				ImageDataRaw data;
				data.timestamp = ptrGrabResult->GetTimeStamp();
				data.source = config->camera_name;
				data.image_raw = std::vector<std::uint8_t>(static_cast<const std::uint8_t*>(ptrGrabResult->GetBuffer()), static_cast<const std::uint8_t*>(ptrGrabResult->GetBuffer()) + ptrGrabResult->GetBufferSize());

				return data;

			} catch (Pylon::TimeoutException const& e) {
				common::println("[BaslerCamerasNode]: ", e.GetDescription());

				continue;
			}
		} while (true);
	}
};