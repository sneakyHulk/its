#pragma once

#include <array>
#include <cstdint>
#include <vector>

struct BirdEyeVisualizationDataPoint {
	std::array<double, 2> position;
	std::array<double, 2> velocity;
	unsigned int id;
	std::uint8_t object_class;
};

struct BirdEyeVisualizationDataPoints {
	std::vector<BirdEyeVisualizationDataPoint> objects;
};