#include <stdexcept>

#include "BaslerCamerasNode.h"
#include "ImageVisualizationNode.h"
#include "PreprocessingNode.h"
#include "common_exception.h"

void aggregate_signal_handler(int signal) {
	common::print("Terminate Pylon...");
	Pylon::PylonTerminate();
	common::println("done!");

	std::_Exit(signal);
}
int main(int argc, char* argv[]) {
	std::signal(SIGINT, aggregate_signal_handler);
	std::signal(SIGTERM, aggregate_signal_handler);
	try {
		common::print("Initialize Pylon...");
		Pylon::PylonInitialize();
		common::println("done!");


		// BaslerCamerasNode cameras({{"s110_s_cam_8", {"0030532A9B7F"}}, {"s110_o_cam_8", {"003053305C72"}}, {"s110_n_cam_8", {"003053305C75"}}, {"s110_w_cam_8", {"003053380639"}}});
		BaslerCamerasNode cameras({{"s060_s_cam_16_k_south", {"00305338063B"}}, {"s060_s_cam_16_k_north", {"0030532A9B7D"}}});
		// PreprocessingNode pre({{"s110_n_cam_8", {1200, 1920, cv::ColorConversionCodes::COLOR_BayerBG2BGR}}, {"s110_w_cam_8", {1200, 1920, cv::ColorConversionCodes::COLOR_BayerBG2BGR}},
		//     {"s110_s_cam_8", {1200, 1920, cv::ColorConversionCodes::COLOR_BayerBG2BGR}}, {"s110_o_cam_8", {1200, 1920, cv::ColorConversionCodes::COLOR_BayerBG2BGR}}});
		// ImageVisualizationNode img([](ImageData const& data) { return data.source == "s110_s_cam_8"; });
		//
		// cameras += pre;
		// pre += img;

		// std::thread cameras_thread(&BaslerCamerasNode::operator(), &cameras);
		// std::thread pre_thread(&PreprocessingNode::operator(), &pre);
		// std::thread img_thread(&ImageVisualizationNode::operator(), &img);
		std::this_thread::sleep_for(10s);
	} catch (...) {
		common::print("Terminate Pylon...");
		Pylon::PylonTerminate();
		common::println("done!");

		rethrow_exception(std::current_exception());
	}

	common::print("Terminate Pylon...");
	Pylon::PylonTerminate();
	common::println("done!");
}