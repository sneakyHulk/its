#include <chrono>
#include <filesystem>
#include <map>

#include "CamerasSimulatorNode.h"
#include "ImageVisualizationNode.h"
#include "PreprocessingNode.h"
#include "RawDataCamerasSimulatorNode.h"

using namespace std::chrono_literals;

int main() {
	CamerasSimulatorNode cams = make_cameras_simulator_node_arrived1({{"s110_n_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "s110_cams" / "s110_n_cam_8" / "s110_n_cam_8_images_distorted"}});

	ImageVisualizationNode img([](ImageData const& data) { return data.source == "s110_n_cam_8"; });

	cams += img;

	std::thread cams_thread(&CamerasSimulatorNode::operator(), &cams);
	std::thread img_thread(&ImageVisualizationNode::operator(), &img);

	RawDataCamerasSimulatorNode raw_cams = make_raw_data_cameras_simulator_node_arrived_recorded1({{"s110_s_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "s110_cams_raw" / "s110_s_cam_8"}});
	PreprocessingNode pre({{"s110_n_cam_8", {1200, 1920, cv::ColorConversionCodes::COLOR_BayerBG2BGR}}, {"s110_w_cam_8", {1200, 1920, cv::ColorConversionCodes::COLOR_BayerBG2BGR}},
	    {"s110_s_cam_8", {1200, 1920, cv::ColorConversionCodes::COLOR_BayerBG2BGR}}, {"s110_o_cam_8", {1200, 1920, cv::ColorConversionCodes::COLOR_BayerBG2BGR}}});
	ImageVisualizationNode raw_img([](ImageData const& data) { return data.source == "s110_s_cam_8"; });

	raw_cams += pre;
	pre += raw_img;

	std::thread raw_cams_thread(&RawDataCamerasSimulatorNode::operator(), &raw_cams);
	std::thread pre_thread(&PreprocessingNode::operator(), &pre);
	std::thread raw_img_thread(&ImageVisualizationNode::operator(), &raw_img);

	std::this_thread::sleep_for(10s);
}