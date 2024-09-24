#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "Detection2D.h"

struct CompactObject {
	unsigned int id;
	std::uint8_t object_class;
	std::array<double, 3> position;  // x, y, z
	std::array<double, 3> angle;     // roll, pitch, yaw
	std::array<double, 3> size;      // height, width, depth
	std::array<double, 3> velocity;  // vx, vy, vz
};

struct CompactObjects {
	std::uint64_t timestamp;             // UTC timestamp since epoch in ns
	std::vector<CompactObject> objects;  // vector of objects
};
