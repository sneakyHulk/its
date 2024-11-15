#pragma once

#include <pylon/BaslerUniversalInstantCamera.h>
#include <pylon/GrabResultPtr.h>
#include <pylon/PylonIncludes.h>

#include <boost/circular_buffer.hpp>
#include <chrono>
#include <thread>
#include <vector>

#include "common_output.h"

using namespace std::chrono_literals;

class BaslerCameraBase {
   protected:
	static void enable_ptp(Pylon::CBaslerUniversalInstantCamera& camera, std::string const& camera_name, std::chrono::nanoseconds max_time_offset) {
	start:
		if (camera.GevIEEE1588.GetValue()) {
			common::println("[BaslerCamerasNode]: ", camera_name, " PTP already enabled!");
		} else {
			common::print("[BaslerCamerasNode]: ", camera_name, " Enable PTP clock synchronization...");
			camera.GevIEEE1588.SetValue(true);
			common::println("done!");
		}

		auto status = Basler_UniversalCameraParams::GevIEEE1588StatusLatchedEnums::GevIEEE1588StatusLatched_Disabled;
		do {
			camera.GevIEEE1588DataSetLatch.Execute();

			status = camera.GevIEEE1588StatusLatched();
			while (status == Basler_UniversalCameraParams::GevIEEE1588StatusLatchedEnums::GevIEEE1588StatusLatched_Initializing) {
				std::this_thread::yield();
				status = camera.GevIEEE1588StatusLatched();
			}

			std::chrono::nanoseconds current_offset = std::chrono::nanoseconds(std::abs(camera.GevIEEE1588OffsetFromMaster()));

			camera.GevTimestampControlLatch.Execute();
			std::chrono::nanoseconds current_server_timestamp = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now()).time_since_epoch();
			std::chrono::nanoseconds current_camera_timestamp = std::chrono::nanoseconds(camera.GevTimestampValue.GetValue());

			common::print("[BaslerCamerasNode]: ", camera_name, " ptp status '");
			switch (status) {
				case Basler_UniversalCameraParams::GevIEEE1588StatusLatchedEnums::GevIEEE1588StatusLatched_Disabled: common::print("GevIEEE1588StatusLatched_Disabled"); break;
				case Basler_UniversalCameraParams::GevIEEE1588StatusLatchedEnums::GevIEEE1588StatusLatched_Master: common::print("GevIEEE1588StatusLatched_Master"); break;
				case Basler_UniversalCameraParams::GevIEEE1588StatusLatchedEnums::GevIEEE1588StatusLatched_Faulty: common::print("GevIEEE1588StatusLatched_Faulty"); break;
				case Basler_UniversalCameraParams::GevIEEE1588StatusLatchedEnums::GevIEEE1588StatusLatched_Initializing: common::print("GevIEEE1588StatusLatched_Initializing"); break;
				case Basler_UniversalCameraParams::GevIEEE1588StatusLatchedEnums::GevIEEE1588StatusLatched_Listening: common::print("GevIEEE1588StatusLatched_Listening"); break;
				case Basler_UniversalCameraParams::GevIEEE1588StatusLatchedEnums::GevIEEE1588StatusLatched_Passive: common::print("GevIEEE1588StatusLatched_Passive"); break;
				case Basler_UniversalCameraParams::GevIEEE1588StatusLatchedEnums::GevIEEE1588StatusLatched_Slave: common::print("GevIEEE1588StatusLatched_Slave"); break;
				case Basler_UniversalCameraParams::GevIEEE1588StatusLatchedEnums::GevIEEE1588StatusLatched_Uncalibrated: common::print("GevIEEE1588StatusLatched_Uncalibrated"); break;
				case Basler_UniversalCameraParams::GevIEEE1588StatusLatchedEnums::GevIEEE1588StatusLatched_Undefined: common::print("GevIEEE1588StatusLatched_Undefined"); break;
				case Basler_UniversalCameraParams::GevIEEE1588StatusLatchedEnums::GevIEEE1588StatusLatched_PreMaster: common::print("GevIEEE1588StatusLatched_PreMaster"); break;
			}
			common::println("' with offset from master '", current_offset, "' and offset from server '",
			    current_server_timestamp < current_camera_timestamp ? current_camera_timestamp - current_server_timestamp : current_server_timestamp - current_camera_timestamp, ".");

			if (status == Basler_UniversalCameraParams::GevIEEE1588StatusLatchedEnums::GevIEEE1588StatusLatched_Master) {
				common::print("[BaslerCamerasNode]: ", camera_name, " restart as slave!");
				camera.GevIEEE1588.SetValue(false);
				std::this_thread::yield();
				goto start;
			}

			std::this_thread::sleep_for(1s);

		} while (status != Basler_UniversalCameraParams::GevIEEE1588StatusLatchedEnums::GevIEEE1588StatusLatched_Slave);

		boost::circular_buffer<std::chrono::nanoseconds> clock_offsets(10, std::chrono::nanoseconds::max());
		do {
			camera.GevIEEE1588DataSetLatch.Execute();

			status = camera.GevIEEE1588StatusLatched();
			while (status == Basler_UniversalCameraParams::GevIEEE1588StatusLatchedEnums::GevIEEE1588StatusLatched_Initializing) {
				std::this_thread::yield();
				status = camera.GevIEEE1588StatusLatched();
			}

			std::chrono::nanoseconds current_offset = std::chrono::nanoseconds(std::abs(camera.GevIEEE1588OffsetFromMaster()));
			clock_offsets.push_back(current_offset);

			std::chrono::nanoseconds current_server_timestamp = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now()).time_since_epoch();
			camera.GevTimestampControlLatch.Execute();
			std::chrono::nanoseconds current_camera_timestamp = std::chrono::nanoseconds(camera.GevTimestampValue.GetValue());

			common::println("[BaslerCameraBase]: ", camera_name, " offset from master '", current_offset, "', max offset over last ", clock_offsets.size(), "s is '", *std::ranges::max_element(clock_offsets), "'. Offset from server is ",
			    current_server_timestamp < current_camera_timestamp ? current_camera_timestamp - current_server_timestamp : current_server_timestamp - current_camera_timestamp, ".");

			std::this_thread::sleep_for(1s);

		} while (*std::ranges::max_element(clock_offsets) > max_time_offset);
	}
};