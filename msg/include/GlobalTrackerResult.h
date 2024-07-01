#pragma once

#include <array>
#include <cstdint>
#include <vector>

struct GlobalTrackerResult {
	unsigned int id;
	std::uint8_t object_class;
	bool matched;
	std::array<double, 2> position;
	std::array<double, 2> velocity;
	double heading;
};

struct GlobalTrackerResults {
	std::uint64_t timestamp;
	std::vector<GlobalTrackerResult> objects;
};