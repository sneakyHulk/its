#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <regex>

#include "ImageData.h"
#include "ImageDataRaw.h"
#include "node.h"

class BayerBG8CameraSimulator : public InputNode<ImageDataRaw> {
	struct ArrivedRecordedSourceConfig {
		std::uint64_t arrived;
		std::uint64_t recorded;
		std::string source;
	};

	std::vector<ArrivedRecordedSourceConfig> _files;
	std::map<std::string, std::filesystem::path> _folders;
	std::chrono::time_point<std::chrono::system_clock> _images_time;
	std::chrono::time_point<std::chrono::system_clock> _current_time;
	std::priority_queue<ArrivedRecordedSourceConfig, std::vector<ArrivedRecordedSourceConfig>, decltype([](ArrivedRecordedSourceConfig const& lhs, ArrivedRecordedSourceConfig const& rhs) { return lhs.arrived > rhs.arrived; })> _queue;

   public:
	explicit BayerBG8CameraSimulator(std::map<std::string, std::filesystem::path>&& folders) : _folders(std::move(folders)) {
		for (auto const& [source, folder] : _folders) {
			for (auto const& file : std::filesystem::directory_iterator(folder)) {
				static std::regex const timestamp("([0-9]+)_([0-9]+)");
				std::cmatch m;
				std::string filename = file.path().filename();
				std::regex_match(filename.c_str(), m, timestamp);
				std::string a = m[0];
				std::string ab = m[1];
				std::string b = m[2];
				_files.emplace_back(std::stoull(m[1].str()), std::stoull(m[2].str()), source);
			}
		}
	}

	ImageDataRaw input_function() final {
		if (_queue.empty()) {
			_queue = decltype(_queue)(_files.begin(), _files.end());

			auto const [arrived, recorded, source] = _queue.top();

			std::this_thread::sleep_for(1s);

			_images_time = std::chrono::time_point<std::chrono::system_clock>(std::chrono::nanoseconds(arrived));
			_current_time = std::chrono::system_clock::now();
		}

		auto const [arrived, recorded, source] = _queue.top();
		_queue.pop();

		std::chrono::time_point<std::chrono::system_clock> next = std::chrono::time_point<std::chrono::system_clock>(std::chrono::nanoseconds(arrived));

		ImageDataRaw data;
		std::ifstream in(_folders[source] / (std::to_string(arrived) + '_' + std::to_string(recorded)), std::ios::binary);
		data.image_raw = std::vector<std::uint8_t>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
		data.source = source;
		std::this_thread::sleep_until(_current_time + (next - _images_time));

		data.timestamp = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now()).time_since_epoch().count();
		// data.timestamp = (std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now()) + std::chrono::nanoseconds(recorded) - std::chrono::nanoseconds(arrived)).time_since_epoch().count();

		return data;
	}
};
