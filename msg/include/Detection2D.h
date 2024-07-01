#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct BoundingBoxXYXY {
	double left;
	double top;
	double right;
	double bottom;
};

struct Detection2D {
	BoundingBoxXYXY bbox;
	double conf;
	std::uint8_t object_class;
};

struct Detections2D {
	std::uint64_t timestamp;           // UTC timestamp since epoch in ns
	std::string source;                // sensor source of detections
	std::vector<Detection2D> objects;  // vector of detections
};