#include "undistort_node.h"

#include <tuple>

UndistortDetections::UndistortDetections(Config const& config) : config(config) {}
Detections2D UndistortDetections::function(const Detections2D& data) {
	Detections2D undistorted_data = data;

	for (auto& detection : undistorted_data.objects) {
		auto& [left, top, right, bottom] = detection.bbox;

		std::tie(left, top) = config.undistort_point(data.source, left, top);
		std::tie(right, bottom) = config.undistort_point(data.source, right, bottom);
	}

	return undistorted_data;
}
