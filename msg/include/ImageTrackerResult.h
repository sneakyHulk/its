#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "Detection2D.h"

struct ImageTrackerResult {
	BoundingBoxXYXY bbox;
	std::array<double, 2> position;
	std::array<double, 2> velocity;
	unsigned int id;
	std::uint8_t object_class;
	bool matched;
};

struct ImageTrackerResults {
	std::uint64_t timestamp;                  // UTC timestamp since epoch in ns
	std::string source;                       // sensor source of detections
	std::vector<ImageTrackerResult> objects;  // vector of tracks
};