#pragma once

#include <pylon/BaslerUniversalInstantCameraArray.h>
#include <pylon/GrabResultPtr.h>
#include <pylon/PylonIncludes.h>

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
	inline static class PylonRAII {
		static void pylon_uninitialization_handler(int signal) {
			common::println("[PylonRAII]: Pylon::PylonTerminate() via pylon_uninitialization_handler()");
			Pylon::PylonTerminate();
			std::_Exit(signal);
		}

	   public:
		PylonRAII() {
			common::println("[PylonRAII]: Pylon::PylonInitialize()");
			std::signal(SIGTERM, pylon_uninitialization_handler);
			std::signal(SIGABRT, pylon_uninitialization_handler);
			std::signal(SIGINT, pylon_uninitialization_handler);
			Pylon::PylonInitialize();
		}

		~PylonRAII() {
			common::println("[PylonRAII]: Pylon::PylonTerminate() via ~PylonRAII()");
			Pylon::PylonTerminate();
		}
	} pylon_initialization;

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

	explicit BaslerCamerasNode(std::map<std::string, MacAddressConfig>&& mac_addresses) {
		auto index = 0;
		for (auto const& [cam_name, mac_address] : mac_addresses) {
			_cam_name_mac_address_index_map.emplace(cam_name, mac_address.address, index++);
		}
		_cameras.Initialize(index);

		Pylon::CDeviceInfo info;
		info.SetDeviceClass(Pylon::BaslerGigEDeviceClass);

		Pylon::DeviceInfoList device_list;
		Pylon::CTlFactory::GetInstance().EnumerateDevices(device_list);
		if (device_list.empty()) {
			throw common::Exception("[BaslerCamerasNode]: No Basler devices found!");
		}

		try {
			for (auto& mac_address_indexing = _cam_name_mac_address_index_map.get<CameraNameMacAddressIndexConfig::MacAddressTag>(); Pylon::CDeviceInfo & device : device_list) {
				auto& config = *mac_address_indexing.find(device.GetMacAddress().c_str());

				_cameras[config.index].Attach(Pylon::CTlFactory::GetInstance().CreateDevice(device));
				common::println("[BaslerCamerasNode]: Found device ", config.camera_name, " with model name '", device.GetModelName(), "', ip address '", device.GetIpAddress(), "', and mac address '", device.GetMacAddress(), "'.");
				std::this_thread::sleep_for(1s);
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
	    _cam_name_mac_address_index_map;

	ImageDataRaw input_function() final {
		ImageDataRaw data;

		return data;
	}
};

// index -> cam_name
//     cam_name->mac_address
//     mac_address->cam_name