#include "RawDataCamerasSimulatorNode.h"

ImageDataRaw RawDataCamerasSimulatorNode::input_function() {
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

	ImageDataRaw data;
	std::ifstream in(path, std::ios::binary);
	data.image_raw = std::vector<std::uint8_t>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
	data.source = source;
	std::this_thread::sleep_until(_current_time + (next - _images_time));

	data.timestamp = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now()).time_since_epoch().count();
	// data.timestamp = (std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now()) + std::chrono::nanoseconds(recorded) - std::chrono::nanoseconds(arrived)).time_since_epoch().count();

	return data;
}
