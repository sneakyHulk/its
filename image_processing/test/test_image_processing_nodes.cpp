#include <chrono>

#include "ImageDownscalingNode.h"
#include "ImagePreprocessingNode.h"
#include "ImageUndistortionNode.h"
#include "ImageVisualizationNode.h"
#include "RawDataCamerasSimulatorNode.h"
#include "config.h"

using namespace std::chrono_literals;

int main(int argc, char** argv) {
	gtk_init(&argc, &argv);

	RawDataCamerasSimulatorNode raw_cams = make_raw_data_cameras_simulator_node_arrived_recorded1({{"s110_n_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "s110_cams_raw" / "s110_n_cam_8"}});
	ImagePreprocessingNode pre({{"s110_n_cam_8", {1200, 1920, cv::ColorConversionCodes::COLOR_BayerBG2BGR}}, {"s110_w_cam_8", {1200, 1920, cv::ColorConversionCodes::COLOR_BayerBG2BGR}},
	    {"s110_s_cam_8", {1200, 1920, cv::ColorConversionCodes::COLOR_BayerBG2BGR}}, {"s110_o_cam_8", {1200, 1920, cv::ColorConversionCodes::COLOR_BayerBG2BGR}}});
	ImageUndistortionNode undist(
	    {{"s110_n_cam_8", {config::intrinsic_matrix_s110_n_cam_8, config::distortion_values_s110_n_cam_8, config::optimal_camera_matrix_s110_n_cam_8, config::undistortion_map1_s110_n_cam_8, config::undistortion_map2_s110_n_cam_8}}});
	ImageDownscalingNode<640, 640> down;

	ImageVisualizationNode raw_img([](ImageData const& data) { return data.source == "s110_n_cam_8"; });
	ImageVisualizationNode down_img([](ImageData const& data) { return data.source == "s110_n_cam_8"; });

	raw_cams.asynchronously_connect(pre);
	pre.synchronously_connect(raw_img);
	pre.synchronously_connect(undist).synchronously_connect(raw_img);
	pre.synchronously_connect(down);
	undist.synchronously_connect(down).synchronously_connect(down_img);

	auto raw_cams_thread = raw_cams();
	auto pre_thread = pre();

	for (auto timestamp = std::chrono::system_clock::now() + 10s; std::chrono::system_clock::now() < timestamp; std::this_thread::yield()) g_main_context_iteration(NULL, true);
}