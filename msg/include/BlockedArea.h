#pragma once

#include <boost/geometry/geometries/geometries.hpp>
#include <cstdint>

struct BlockedArea {
	boost::geometry::model::ring<boost::geometry::model::point<double, 2, boost::geometry::cs::cartesian>> area;
};

struct  BlockedAreas {
	std::uint64_t timestamp;
	std::vector<BlockedArea> areas;
};