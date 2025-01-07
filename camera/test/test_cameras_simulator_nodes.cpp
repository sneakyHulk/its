#include <chrono>
#include <filesystem>
#include <map>

#include "CamerasSimulatorNode.h"
#include "ImagePreprocessingNode.h"
#include "ImageVisualizationNode.h"
#include "RawDataCamerasSimulatorNode.h"
#include "common_output.h"

using namespace std::chrono_literals;

int main(int argc, char* argv[]) {
	gtk_init(&argc, &argv);

	{
		CamerasSimulatorNode cams = make_cameras_simulator_node_arrived1({{"s110_n_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "s110_cams" / "s110_n_cam_8" / "s110_n_cam_8_images_distorted"}});
		ImageVisualizationNode img([](ImageData const& data) { return data.source == "s110_n_cam_8"; });

		cams.synchronously_connect(img);

		auto cameras_thread = cams();

		for (auto timestamp = std::chrono::system_clock::now() + 10s; std::chrono::system_clock::now() < timestamp; std::this_thread::yield()) g_main_context_iteration(NULL, true);
	}

	{
		RawDataCamerasSimulatorNode raw_cams = make_raw_data_cameras_simulator_node_arrived_recorded1({{"s110_s_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "s110_cams_raw" / "s110_s_cam_8"}});
		ImagePreprocessingNode pre({{"s110_n_cam_8", {1200, 1920, cv::ColorConversionCodes::COLOR_BayerBG2BGR}}, {"s110_w_cam_8", {1200, 1920, cv::ColorConversionCodes::COLOR_BayerBG2BGR}},
		    {"s110_s_cam_8", {1200, 1920, cv::ColorConversionCodes::COLOR_BayerBG2BGR}}, {"s110_o_cam_8", {1200, 1920, cv::ColorConversionCodes::COLOR_BayerBG2BGR}}});
		ImageVisualizationNode raw_img([](ImageData const& data) { return data.source == "s110_s_cam_8"; });

		raw_cams.asynchronously_connect(pre);
		pre.synchronously_connect(raw_img);

		auto raw_cameras_thread = raw_cams();
		auto pre_thread = pre();

		for (auto timestamp = std::chrono::system_clock::now() + 10s; std::chrono::system_clock::now() < timestamp; std::this_thread::yield()) g_main_context_iteration(NULL, true);
	}
}