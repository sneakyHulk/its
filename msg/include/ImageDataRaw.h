#include <cstdint>
#include <string>
#include <vector>

struct ImageDataRaw {
	std::vector<std::uint8_t> image_raw;
	std::uint64_t timestamp;
	std::string source;
};