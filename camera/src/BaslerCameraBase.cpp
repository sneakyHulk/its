#include "BaslerCameraBase.h"

#include <boost/circular_buffer.hpp>
#include <thread>

template <bool v2>
void BaslerCameraBase<v2>::enable_ptp(Pylon::CBaslerUniversalInstantCamera const& camera, std::string const& camera_name, std::chrono::nanoseconds max_time_offset) {
start:
	if (camera.GevIEEE1588.GetValue()) {
		common::println_debug_loc(camera_name, " PTP already enabled!");
	} else {
		common::print_debug_loc(camera_name, " Enable PTP clock synchronization...");
		camera.GevIEEE1588.SetValue(true);
		common::println_debug("done!");
	}

	auto status = Basler_UniversalCameraParams::GevIEEE1588StatusLatchedEnums::GevIEEE1588StatusLatched_Disabled;
	// Puts the camera in PTP Salve mode.
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

		common::print_debug_loc(camera_name, " ptp status '");
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
		common::println_debug("' with offset from master '", current_offset, "' and offset from server '",
		    current_server_timestamp < current_camera_timestamp ? current_camera_timestamp - current_server_timestamp : current_server_timestamp - current_camera_timestamp, ".");

		// We want the server to be the PTP master, not some Basler camera.
		if (status == Basler_UniversalCameraParams::GevIEEE1588StatusLatchedEnums::GevIEEE1588StatusLatched_Master) {
			common::println_loc(camera_name, " restart as slave!");
			camera.GevIEEE1588.SetValue(false);
			std::this_thread::yield();
			goto start;
		}

		std::this_thread::sleep_for(1s);

	} while (status != Basler_UniversalCameraParams::GevIEEE1588StatusLatchedEnums::GevIEEE1588StatusLatched_Slave);

	// on ace2 cameras, see https://docs.baslerweb.com/precision-time-protocol#additional-parameters
	if constexpr (v2) {
		auto ptp_status = Basler_UniversalCameraParams::PtpServoStatusEnums::PtpServoStatus_Unknown;

		do {
			std::this_thread::yield();

			camera.PtpDataSetLatch.Execute();

			ptp_status = camera.PtpServoStatus.GetValue();
		} while (ptp_status != Basler_UniversalCameraParams::PtpServoStatusEnums::PtpServoStatus_Locked);

		return;
	}

	boost::circular_buffer<std::chrono::nanoseconds> clock_offsets(10, std::chrono::nanoseconds::max());
	// Ensures that the time offset between the camera and the server acting as the PTP master is sufficiently synchronized, as specified in max_time_offset. https://docs.baslerweb.com/precision-time-protocol
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

		common::println_debug_loc(camera_name, " offset from master '", current_offset, "', max offset over last ", clock_offsets.size(), "s is '", *std::ranges::max_element(clock_offsets), "'. Offset from server is ",
		    current_server_timestamp < current_camera_timestamp ? current_camera_timestamp - current_server_timestamp : current_server_timestamp - current_camera_timestamp, ".");

		std::this_thread::sleep_for(1s);

	} while (*std::ranges::max_element(clock_offsets) > max_time_offset);
}

template class BaslerCameraBase<true>;
template class BaslerCameraBase<false>;