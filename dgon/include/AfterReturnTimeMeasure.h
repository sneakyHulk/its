#pragma once
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <source_location>
#include <utility>

class AfterReturnTimeMeasure {
	std::chrono::time_point<std::chrono::system_clock> const t0;
	std::source_location const location;
	static std::map<std::string, std::ofstream> log_files;
	static std::filesystem::path output_folder;
	std::string options;

	void init() {
		std::string filename = std::filesystem::path(location.file_name()).stem();
		auto& file = log_files[options + filename];

		if (!file.is_open()) {
			std::cout << "open: " << output_folder / filename << std::endl;
			file = std::ofstream(output_folder / (filename + '_' + options));
		}
	}

   public:
	explicit AfterReturnTimeMeasure(std::chrono::time_point<std::chrono::system_clock> t0, std::source_location const location = std::source_location::current()) : t0(t0), location(location) { init(); }
	explicit AfterReturnTimeMeasure(std::uint64_t t0, std::source_location const location = std::source_location::current()) : t0(std::chrono::nanoseconds(t0)), location(location) { init(); }
	explicit AfterReturnTimeMeasure(std::uint64_t t0, std::string&& options, std::source_location const location = std::source_location::current()) : t0(std::chrono::nanoseconds(t0)), location(location), options(std::move(options)) {
		init();
	}
	~AfterReturnTimeMeasure() {
		auto t1 = std::chrono::system_clock::now();
		log_files.at(options + std::filesystem::path(location.file_name()).stem().string()) << std::chrono::duration<std::uint64_t, std::nano>(t1 - t0).count() << std::endl;
	}
};