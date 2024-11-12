#include <cstdlib>
#include <stdexcept>

#include "BaslerCamerasNode.h"
#include "ImageVisualizationNode.h"
#include "PreprocessingNode.h"
#include "SavingNode.h"
#include "common_exception.h"

std::function<void()> clean_up;

void signal_handler(int signal) {
	common::println("[signal_handler]: got signal '", signal, "'!");
	clean_up();
}

int main(int argc, char* argv[]) {
	clean_up = []() {
		common::print("Terminate Pylon...");
		Pylon::PylonTerminate();
		common::println("done!");

		std::exception_ptr current_exception = std::current_exception();
		if (current_exception)
			std::rethrow_exception(current_exception);
		else
			std::_Exit(1);
	};

	std::signal(SIGINT, signal_handler);
	std::signal(SIGTERM, signal_handler);
	try {
		common::print("Initialize Pylon...");
		Pylon::PylonInitialize();
		common::println("done!");

		// BaslerCamerasNode cameras({{"s110_s_cam_8", {"0030532A9B7F"}}, {"s110_o_cam_8", {"003053305C72"}}, {"s110_n_cam_8", {"003053305C75"}}, {"s110_w_cam_8", {"003053380639"}}});
		// PreprocessingNode pre({{"s110_n_cam_8", {1200, 1920, cv::ColorConversionCodes::COLOR_BayerBG2BGR}}, {"s110_w_cam_8", {1200, 1920, cv::ColorConversionCodes::COLOR_BayerBG2BGR}},
		//     {"s110_s_cam_8", {1200, 1920, cv::ColorConversionCodes::COLOR_BayerBG2BGR}}, {"s110_o_cam_8", {1200, 1920, cv::ColorConversionCodes::COLOR_BayerBG2BGR}}});
		// ImageVisualizationNode img([](ImageData const& data) { return data.source == "s110_s_cam_8"; });

		BaslerCamerasNode cameras({{"s60_n_cam_16_k", {"00305338063B"}}, {"s60_n_cam_50_k", {"0030532A9B7D"}}});
		PreprocessingNode pre({{"s60_n_cam_16_k", {1200, 1920, cv::ColorConversionCodes::COLOR_BayerBG2BGR}}, {"s60_n_cam_50_k", {1200, 1920, cv::ColorConversionCodes::COLOR_BayerBG2BGR}}});
		SavingImageDataNode save({{"s60_n_cam_16_k", {std::filesystem::path(CMAKE_SOURCE_DIR) / "result" / "s60_n_cam_16_k"}}, {"s60_n_cam_50_k", {std::filesystem::path(CMAKE_SOURCE_DIR) / "result" / "s60_n_cam_50_k"}}});

		// BaslerCamerasNode cameras({{"car_cam_16", BaslerCamerasNode::MacAddressConfig{"0030534C1B61"}}});
		// PreprocessingNode pre({{"car_cam_16", {1200, 1920, cv::ColorConversionCodes::COLOR_BayerBG2BGR}}});
		// SavingImageDataNode save({{"car_cam_16", {std::filesystem::path(CMAKE_SOURCE_DIR) / "result" / "car_cam_16"}}});

		cameras += pre;
		pre += save;

		cameras();
		pre();
		save();

		std::this_thread::sleep_for(20s);

		clean_up();
	} catch (...) {
		clean_up();
	}
}