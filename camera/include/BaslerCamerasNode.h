#pragma once

#include <pylon/BaslerUniversalInstantCameraArray.h>

#include <boost/circular_buffer.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>
#include <chrono>

#include "BaslerCameraBase.h"
#include "ImageDataRaw.h"
#include "Pusher.h"
#include "common_output.h"

/**
 * @class BaslerCamerasNode
 * @brief Manages multiple Basler cameras.
 *
 * This class handles initialization, ptp configuration, and image capture for multiple basler GigE cameras.
 */
class BaslerCamerasNode : public Pusher<ImageDataRaw>, public BaslerCameraBase {
   public:
	struct MacAddressConfig {
		std::string address;
		double fps;
	};

	/**
	 * @struct CameraNameMacAddressIndexConfig
	 * @brief Configuration structure for camera indexing based on name and MAC address and index in BaslerUniversalInstantCameraArray.
	 */
	struct CameraNameMacAddressIndexConfig {
		struct CameraNameTag {};
		struct MacAddressTag {};
		struct IndexTag {};
		std::string camera_name;
		std::string mac_address;
		int index;
		double fps;
	};

	/**
	 * Trys to map the given mac addresses to the addresses of the available basler cameras found by pylon.
	 * Then configures ptp and initialize image grabbing with the specified cameras, that were also found.
	 *
	 * @param camera_name_mac_address A map which specifies the different cameras with their mac addresses that should perform image capture.
	 */
	explicit BaslerCamerasNode(std::map<std::string, MacAddressConfig>&& camera_name_mac_address);

   private:
	Pylon::CBaslerUniversalInstantCameraArray _cameras;

	/**
	 * Container for camera indexing based on name and MAC address and index in BaslerUniversalInstantCameraArray.
	 */
	boost::multi_index_container<CameraNameMacAddressIndexConfig, boost::multi_index::indexed_by<boost::multi_index::ordered_unique<boost::multi_index::tag<BaslerCamerasNode::CameraNameMacAddressIndexConfig::CameraNameTag>,
	                                                                                                 boost::multi_index::member<CameraNameMacAddressIndexConfig, std::string, &CameraNameMacAddressIndexConfig::camera_name>>,
	                                                                  boost::multi_index::ordered_unique<boost::multi_index::tag<BaslerCamerasNode::CameraNameMacAddressIndexConfig::MacAddressTag>,
	                                                                      boost::multi_index::member<CameraNameMacAddressIndexConfig, std::string, &CameraNameMacAddressIndexConfig::mac_address>>,
	                                                                  boost::multi_index::ordered_unique<boost::multi_index::tag<BaslerCamerasNode::CameraNameMacAddressIndexConfig::IndexTag>,
	                                                                      boost::multi_index::member<CameraNameMacAddressIndexConfig, int, &CameraNameMacAddressIndexConfig::index>>>>
	    _camera_name_mac_address_index_map;

	/**
	 * @brief Captures and processes images from the cameras.
	 * @return Captured image data in the form of ImageDataRaw.
	 */
	ImageDataRaw push() final;
};