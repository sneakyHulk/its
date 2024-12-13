#include "camera_simulator_node.h"

#include <chrono>
using namespace std::chrono_literals;

CameraSimulator::CameraSimulator(std::string const& cam_name) {
	files = std::vector(std::filesystem::recursive_directory_iterator(std::filesystem::path(CMAKE_SOURCE_DIR) / std::filesystem::path("data/camera") / std::filesystem::path(cam_name)), {});
	files.erase(std::remove_if(files.begin(), files.end(), [](auto const& e) { return e.path().extension() != ".jpg"; }), files.end());
	files.erase(std::remove_if(files.begin(), files.end(), [](auto const& e) { return e.path().generic_string().find("_distorted") == std::string::npos; }), files.end());

	std::sort(files.begin(), files.end(), [](auto const& e1, auto const& e2) { return e1.path().stem() > e2.path().stem(); });
	common::println("Dealing with ", files.size(), " files!");
}

ImageData CameraSimulator::input_function() {
	static std::chrono::time_point<std::chrono::system_clock> images_time;
	static std::chrono::time_point<std::chrono::system_clock> current_time = std::chrono::system_clock::now();
	while (files_copy.empty()) {
		common::println("Starting to push frames ...");
		files_copy = files;
		std::this_thread::sleep_for(1s);

		images_time = std::chrono::time_point<std::chrono::system_clock>(std::chrono::milliseconds(std::stoll(files.back().path().stem())));
		current_time = std::chrono::system_clock::now();
	}

	auto current_file = files_copy.back();
	files_copy.pop_back();

	std::chrono::time_point<std::chrono::system_clock> next(std::chrono::milliseconds(std::stoll(current_file.path().stem())));  // image name in microseconds

	ImageData data;
	data.image = cv::imread(current_file.path(), cv::IMREAD_COLOR);
	data.source = current_file.path().parent_path().parent_path().filename();

	std::this_thread::sleep_until(current_time + (next - images_time));

	data.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::milliseconds(std::stoll(current_file.path().stem())))
	                     .count();  // std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now()).time_since_epoch().count();

	return data;
}