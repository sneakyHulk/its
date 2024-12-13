#pragma once

#include "BaslerCameraBase.h"
#include "ImageDataRaw.h"
#include "Pusher.h"

/**
 * @class BaslerCameraNode
 * @brief Manages one Basler cameras.
 * This class handles initialization, ptp configuration, and image capture for one basler GigE cameras.
 */
class BaslerCameraNode : public Pusher<ImageDataRaw>, public BaslerCameraBase {
	Pylon::CBaslerUniversalInstantCamera _camera;

	std::string _camera_name;

   public:
	/**
	 * Trys to map the given mac addresses to the addresses of the available basler cameras found by pylon.
	 * Then configures ptp and initialize image grabbing with the specified camera.
	 *
	 * @param camera_name The camera name for debugging and source definitions.
	 * @param mac_address The mac address for the mapping operation.
	 */
	BaslerCameraNode(std::string&& camera_name, std::string&& mac_address);

	/**
	 * Does the same BaslerCameraNode(std::string&& camera_name, std::string&& mac_address). But also set the FPS of the camera.
	 *
	 * @param camera_name The camera name for debugging and source definitions.
	 * @param mac_address The mac address for the mapping operation.
	 * @param fps The fps value to set the camera to.
	 */
	BaslerCameraNode(std::string&& camera_name, std::string&& mac_address, double const fps);

   private:
	/**
	 * @brief Captures and processes images from the cameras.
	 * @return Captured image data in the form of ImageDataRaw.
	 */
	ImageDataRaw push() final;
};