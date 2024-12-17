#pragma once

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <source_location>

class AfterReturnTimeMeasure {
	std::chrono::time_point<std::chrono::system_clock> const t0;
	std::source_location const location;
	static std::map<std::string, std::ofstream> log_files;
	static std::filesystem::path output_folder;

	void init() {
		std::string filename = std::filesystem::path(location.file_name()).stem();
		auto& file = log_files[filename];

		if (!file.is_open()) {
			std::cout << "open: " << output_folder / filename << std::endl;
			file = std::ofstream(output_folder / filename);
		}
	}

   public:
	explicit AfterReturnTimeMeasure(std::chrono::time_point<std::chrono::system_clock> const t0, std::source_location const location = std::source_location::current()) : t0(t0), location(location) { init(); }
	explicit AfterReturnTimeMeasure(std::uint64_t const t0, std::source_location const location = std::source_location::current()) : t0(std::chrono::nanoseconds(t0)), location(location) { init(); }

	~AfterReturnTimeMeasure() {
		auto const t1 = std::chrono::system_clock::now();
		log_files.at(std::filesystem::path(location.file_name()).stem()) << std::chrono::duration<std::uint64_t, std::nano>(t1 - t0).count() << std::endl;
	}
};