#include "AfterReturnTimeMeasure.h"

std::map<std::string, std::ofstream> AfterReturnTimeMeasure::log_files = {};
std::filesystem::path AfterReturnTimeMeasure::output_folder = std::filesystem::path(CMAKE_SOURCE_DIR) / "result";