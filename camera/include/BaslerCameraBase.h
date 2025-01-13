#pragma once

#include <pylon/BaslerUniversalInstantCamera.h>

#include <chrono>

#include "common_output.h"

using namespace std::chrono_literals;

/**
 * @class BaslerCameraBase
 *
 * This class implements common functionality of Basler camera nodes, in particular the ptp configuration.
 *
 * @tparam True if the camera is a ACE 2 camera.
 */
template <bool v2>
class BaslerCameraBase {
   protected:
	/**
	 * @brief Enables PTP (Precision Time Protocol) clock synchronization for a Basler camera.
	 *
	 * @param camera The Basler camera object.
	 * @param camera_name Name of the camera for logging purposes.
	 * @param max_time_offset Maximum allowable time offset of the ptp synchronization.
	 */
	static void enable_ptp(Pylon::CBaslerUniversalInstantCamera const& camera, std::string const& camera_name, std::chrono::nanoseconds max_time_offset);
};