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
#include "node.h"

class CamerasSimulatorNode : public InputNode<ImageData> {
   public:
	struct FilepathArrivedRecordedSourceConfig {
		std::filesystem::path filepath;
		std::uint64_t arrived;
		std::uint64_t recorded;
		std::string source;
	};

	explicit CamerasSimulatorNode(std::vector<FilepathArrivedRecordedSourceConfig>&& files) : _files(std::forward<decltype(files)>(files)) {}

   private:
	ImageData input_function() final;

	struct sorting_function {
		bool operator()(FilepathArrivedRecordedSourceConfig const& lhs, FilepathArrivedRecordedSourceConfig const& rhs) const { return lhs.arrived > rhs.arrived; }
	};

	std::vector<FilepathArrivedRecordedSourceConfig> _files;
	std::chrono::time_point<std::chrono::system_clock> _images_time;
	std::chrono::time_point<std::chrono::system_clock> _current_time;
	std::priority_queue<FilepathArrivedRecordedSourceConfig, std::vector<FilepathArrivedRecordedSourceConfig>, sorting_function> _queue;
};

CamerasSimulatorNode make_cameras_simulator_node_arrived_recorded1(std::map<std::string, std::filesystem::path>&& folders) {
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

	return CamerasSimulatorNode{std::move(ret)};
}

CamerasSimulatorNode make_cameras_simulator_node_arrived1(std::map<std::string, std::filesystem::path>&& folders) {
	std::vector<CamerasSimulatorNode::FilepathArrivedRecordedSourceConfig> ret;

	for (auto const& [source, folder] : folders) {
		for (auto const& file : std::filesystem::directory_iterator(folder)) {
			ret.emplace_back(file.path(), std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::milliseconds(std::stoull(file.path().stem()))).count(),
			    std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::milliseconds(std::stoull(file.path().stem()))).count(), source);
		}
	}

	return CamerasSimulatorNode{std::forward<decltype(ret)>(ret)};
}