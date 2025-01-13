#include <csignal>
#include <cstdlib>
#include <stdexcept>

#include "BaslerCamerasNode.h"
#include "ImagePreprocessingNode.h"
#include "ImageSavingNode.h"
#include "ImageVisualizationNode.h"

void clean_up(int signal) {
	common::println("[clean_up]: got signal '", signal, "'!");

	common::print("[clean_up]: Terminate Pylon...");
	Pylon::PylonTerminate();
	common::println("done!");
}

int main(int argc, char* argv[]) {
	std::signal(SIGINT, [](int signal) {
		clean_up(signal);

		std::_Exit(1);
	});
	std::signal(SIGTERM, [](int signal) {
		clean_up(signal);

		std::_Exit(1);
	});

	common::print("Initialize Pylon...");
	Pylon::PylonInitialize();
	common::println("done!");

	// BaslerCamerasNode cameras({{"s110_s_cam_8", {"0030532A9B7F"}}, {"s110_o_cam_8", {"003053305C72"}}, {"s110_n_cam_8", {"003053305C75"}}, {"s110_w_cam_8", {"003053380639"}}});
	// ImagePreprocessingNode pre({{"s110_n_cam_8", {1200, 1920, cv::ColorConversionCodes::COLOR_BayerBG2BGR}}, {"s110_w_cam_8", {1200, 1920, cv::ColorConversionCodes::COLOR_BayerBG2BGR}},
	//     {"s110_s_cam_8", {1200, 1920, cv::ColorConversionCodes::COLOR_BayerBG2BGR}}, {"s110_o_cam_8", {1200, 1920, cv::ColorConversionCodes::COLOR_BayerBG2BGR}}});
	// ImageSavingNode img([](ImageData const& data) { return data.source == "s110_s_cam_8"; });

	BaslerCamerasNode cameras({{"s60_n_cam_16_k", {"00305338063B"}}, {"s60_n_cam_50_k", {"0030532A9B7D"}}});
	ImagePreprocessingNode pre({{"s60_n_cam_16_k", {1200, 1920, cv::ColorConversionCodes::COLOR_BayerBG2BGR}}, {"s60_n_cam_50_k", {1200, 1920, cv::ColorConversionCodes::COLOR_BayerBG2BGR}}});
	ImageSavingNode save({{"s60_n_cam_16_k", {std::filesystem::path(CMAKE_SOURCE_DIR) / "result" / "s60_n_cam_16_k"}}, {"s60_n_cam_50_k", {std::filesystem::path(CMAKE_SOURCE_DIR) / "result" / "s60_n_cam_50_k"}}});

	// BaslerCamerasNode cameras({{"car_cam_16", BaslerCamerasNode::MacAddressConfig{"0030534C1B61"}}});
	// ImagePreprocessingNode pre({{"car_cam_16", {1200, 1920, cv::ColorConversionCodes::COLOR_BayerBG2BGR}}});
	// ImageSavingNode save({{"car_cam_16", {std::filesystem::path(CMAKE_SOURCE_DIR) / "result" / "car_cam_16"}}});

	cameras.asynchronously_connect(pre);
	pre.asynchronously_connect(save);

	auto cameras_thread = cameras();
	auto pre_thread = pre();
	auto save_thread = save();

	std::this_thread::sleep_for(20s);

	clean_up(0);
}