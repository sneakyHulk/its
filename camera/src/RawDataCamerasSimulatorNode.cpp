#include "RawDataCamerasSimulatorNode.h"

using namespace std::chrono_literals;

/**
 * Sets up a queue that sorts the raw images by the time they arrived when they were taken.
 * The queue then pops the image filename and its additional information one at a time, waiting for the same amount of time that elapsed between the arrival of the raw images.
 * After waiting, it reads the raw image into the return value.
 * When the queue is empty, the process is restarted.
 *
 * @return Image data in the form of ImageDataRaw i.e. in BayerRG8 form.
 */
ImageDataRaw RawDataCamerasSimulatorNode::push() {
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

	if (next - _images_time > 5s) {
		_images_time = std::chrono::time_point<std::chrono::system_clock>(std::chrono::nanoseconds(arrived));
		_current_time = std::chrono::system_clock::now() + 1s;
	}

	std::this_thread::sleep_until(_current_time + (next - _images_time));

	data.timestamp = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now()).time_since_epoch().count();
	// data.timestamp = (std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now()) + std::chrono::nanoseconds(recorded) - std::chrono::nanoseconds(arrived)).time_since_epoch().count();

	return data;
}

/**
 * @brief Initializes a vector containing the image filenames and their additional information coming from a factory method.
 *
 * @param files The vector of image filenames and their additional information.
 */
RawDataCamerasSimulatorNode::RawDataCamerasSimulatorNode(std::vector<FilepathArrivedRecordedSourceConfig>&& files) : _files(std::forward<decltype(files)>(files)) {}
