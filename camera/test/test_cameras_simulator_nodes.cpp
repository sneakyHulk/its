#include <chrono>
#include <filesystem>
#include <map>

#include "CamerasSimulatorNode.h"
#include "ImagePreprocessingNode.h"
#include "ImageVisualizationNode.h"
#include "RawDataCamerasSimulatorNode.h"
#include "common_output.h"

using namespace std::chrono_literals;

std::function<void()> clean_up;

void signal_handler(int signal) {
	common::println("[signal_handler]: got signal '", signal, "'!");
	clean_up();
}

int main() {
	clean_up = []() {
		std::exception_ptr current_exception = std::current_exception();
		if (current_exception)
			std::rethrow_exception(current_exception);
		else
			std::_Exit(1);
	};

	try {
		CamerasSimulatorNode cams = make_cameras_simulator_node_arrived1({{"s110_n_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "s110_cams" / "s110_n_cam_8" / "s110_n_cam_8_images_distorted"}});
		ImageVisualizationNode img([](ImageData const& data) { return data.source == "s110_n_cam_8"; });

		cams.synchronously_connect(img);

		cams();

		for (auto timestamp = std::chrono::system_clock::now() + 10s; std::chrono::system_clock::now() < timestamp;) g_main_context_iteration(NULL, true);

		RawDataCamerasSimulatorNode raw_cams = make_raw_data_cameras_simulator_node_arrived_recorded1({{"s110_s_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "s110_cams_raw" / "s110_s_cam_8"}});
		ImagePreprocessingNode pre({{"s110_n_cam_8", {1200, 1920, cv::ColorConversionCodes::COLOR_BayerBG2BGR}}, {"s110_w_cam_8", {1200, 1920, cv::ColorConversionCodes::COLOR_BayerBG2BGR}},
		    {"s110_s_cam_8", {1200, 1920, cv::ColorConversionCodes::COLOR_BayerBG2BGR}}, {"s110_o_cam_8", {1200, 1920, cv::ColorConversionCodes::COLOR_BayerBG2BGR}}});
		ImageVisualizationNode raw_img([](ImageData const& data) { return data.source == "s110_s_cam_8"; });

		raw_cams.asynchronously_connect(pre);
		pre.synchronously_connect(raw_img);

		raw_cams();
		pre();

		for (auto timestamp = std::chrono::system_clock::now() + 10s; std::chrono::system_clock::now() < timestamp;) g_main_context_iteration(NULL, true);

		clean_up();
	} catch (...) {
		clean_up();
	}
}