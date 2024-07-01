#pragma once

#include <cstdint>
#include <opencv2/opencv.hpp>
#include <string>

struct ImageData {
	cv::Mat image;
	std::uint64_t timestamp;  // UTC timestamp since epoch in ns
	int width;
	int height;
	std::string source;
};