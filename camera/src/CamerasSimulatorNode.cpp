#include <opencv2/opencv.hpp>

#include "CamerasSimulatorNode.h"

CamerasSimulatorNode::CamerasSimulatorNode(std::map<std::string, std::filesystem::path> &&folders, std::function<std::vector<FilepathArrivedRecordedSourceConfig>(std::map<std::string, std::filesystem::path> &&)> &&get_data) {
	_files = get_data(std::forward<decltype(folders)>(folders));
}
ImageData CamerasSimulatorNode::input_function() {
	if (_queue.empty()) {
		_queue = decltype(_queue)(_files.begin(), _files.end());

		auto const [path, arrived, recorded, source] = _queue.top();

		std::this_thread::sleep_for(1s);

		_images_time = std::chrono::time_point<std::chrono::system_clock>(std::chrono::nanoseconds(arrived));
		_current_time = std::chrono::system_clock::now();
	}

	auto const [path, arrived, recorded, source] = _queue.top();
	_queue.pop();

	std::chrono::time_point<std::chrono::system_clock> next = std::chrono::time_point<std::chrono::system_clock>(std::chrono::nanoseconds(arrived));

	ImageData data;
	data.image = cv::imread(path);
	data.source = source;
	std::this_thread::sleep_until(_current_time + (next - _images_time));

	data.timestamp = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now()).time_since_epoch().count();
	// data.timestamp = (std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now()) + std::chrono::nanoseconds(recorded) - std::chrono::nanoseconds(arrived)).time_since_epoch().count();

	return data;
}
