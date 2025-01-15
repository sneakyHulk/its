#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <regex>
#include <utility>

#include "ImageData.h"
#include "ImageDataRaw.h"
#include "Pusher.h"
#include "common_output.h"

/**
 * @class CamerasSimulatorNode
 * @brief Simulates multiple cameras to push images to the pipeline.
 *
 * This class is set up with image files, their source camera and timestamps.
 * The class then pushes out the image data as it was obtained when the data was recorded.
 */
class CamerasSimulatorNode : public Pusher<ImageData> {
   public:
	struct FilepathArrivedRecordedSourceConfig {
		std::filesystem::path filepath;
		std::uint64_t arrived;
		std::uint64_t recorded;
		std::string source;
	};

	explicit CamerasSimulatorNode(std::vector<FilepathArrivedRecordedSourceConfig>&& files);

   private:
	ImageData push() final;

	struct sorting_function {
		bool operator()(FilepathArrivedRecordedSourceConfig const& lhs, FilepathArrivedRecordedSourceConfig const& rhs) const { return lhs.arrived > rhs.arrived; }
	};

	std::vector<FilepathArrivedRecordedSourceConfig> _files;
	std::chrono::time_point<std::chrono::system_clock> _images_time;
	std::chrono::time_point<std::chrono::system_clock> _current_time;
	std::priority_queue<FilepathArrivedRecordedSourceConfig, std::vector<FilepathArrivedRecordedSourceConfig>, sorting_function> _queue;
};

/**
 * @brief Factory method that takes a camera name and a folder containing recoded images for each camera to simulate.
 *
 * @attention Expects the files to be in the format <timestamp of image arrival in ns>_<timestamp of image recording in ns>.
 *
 * @return The CamerasSimulatorNode class which simulates the defined cameras.
 */
inline CamerasSimulatorNode make_cameras_simulator_node_arrived_recorded1(std::map<std::string, std::filesystem::path>&& folders) {
	std::vector<CamerasSimulatorNode::FilepathArrivedRecordedSourceConfig> ret;

	for (auto const& [source, folder] : folders) {
		for (auto const& file : std::filesystem::directory_iterator(folder)) {
			static std::regex const timestamp("([0-9]+)_([0-9]+)");
			std::cmatch m;
			std::string regex_string = file.path().filename();
			std::regex_match(regex_string.c_str(), m, timestamp);

			ret.emplace_back(file.path(), std::stoull(m[1].str()), std::stoull(m[2].str()), source);
		}
	}

	if (ret.empty()) common::println_critical_loc("No image files found!");

	return CamerasSimulatorNode{std::move(ret)};
}

/**
 * @brief Factory method that takes a camera name and a folder containing recoded images for each camera to simulate.
 *
 * @attention Expects the files to be in the format <timestamp of image arrival in ns>(.png|.jpeg|...).
 *
 * @return The CamerasSimulatorNode class which simulates the defined cameras.
 */
inline CamerasSimulatorNode make_cameras_simulator_node_arrived1(std::map<std::string, std::filesystem::path>&& folders) {
	std::vector<CamerasSimulatorNode::FilepathArrivedRecordedSourceConfig> ret;

	for (auto const& [source, folder] : folders) {
		for (auto const& file : std::filesystem::directory_iterator(folder)) {
			ret.emplace_back(file.path(), std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::milliseconds(std::stoull(file.path().stem()))).count(),
			    std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::milliseconds(std::stoull(file.path().stem()))).count(), source);
		}
	}

	if (ret.empty()) common::println_critical_loc("No image files found!");

	return CamerasSimulatorNode{std::forward<decltype(ret)>(ret)};
}

/**
 * @brief Factory method that takes a camera name and a folder containing tumtraf images.
 *
 * @attention Expects the files to be in the tumtraffic format.
 * @attention Unfortunately, the timestamps of the recording are sometimes a little bit off. It looks like it is buggy, but it is not the case.
 *
 * @return The CamerasSimulatorNode class which simulates the defined cameras.
 */
inline CamerasSimulatorNode make_cameras_simulator_node_tumtraf(std::multimap<std::string, std::filesystem::path>&& folders) {
	std::vector<CamerasSimulatorNode::FilepathArrivedRecordedSourceConfig> ret;

	for (auto const& [source, folder] : folders) {
		for (auto const& file : std::filesystem::directory_iterator(folder)) {
			static std::regex const timestamp("([0-9]+)_([0-9]+)_(s110_camera_basler_(north|east|south(1|2))_8mm|vehicle_camera_basler_16mm).jpg");
			std::cmatch m;
			std::string regex_string = file.path().filename();
			std::regex_match(regex_string.c_str(), m, timestamp);

			ret.emplace_back(file.path(), std::stoull(m[1].str() + m[2].str()), std::stoull(m[1].str() + m[2].str()), source);
		}
	}

	if (ret.empty()) common::println_critical_loc("No image files found!");

	return CamerasSimulatorNode{std::move(ret)};
}