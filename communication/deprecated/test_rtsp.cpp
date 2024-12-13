#include <csignal>
#include <thread>

#include "BaslerCameraNode.h"
#include "BaslerCamerasNode.h"
#include "CamerasSimulatorNode.h"
#include "PreprocessingNode.h"
#include "image_communication_node.h"

std::function<void()> clean_up;

void signal_handler(int signal) {
	common::println("[signal_handler]: got signal '", signal, "'!");
	clean_up();
}

// gst-launch-1.0 rtspsrc location=rtsp://80.155.138.138:2346/s60_n_cam_16_k latency=100 ! queue ! rtph264depay ! h264parse ! avdec_h264 ! videoconvert ! autovideosink
int main(int argc, char **argv) {
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

		// BaslerCamerasNode cameras({{"s60_n_cam_16_k", {"00305338063B"}}, {"s60_n_cam_50_k", {"0030532A9B7D"}}});
		BaslerCameraNode camera1("s60_n_cam_16_k", "00305338063B");
		BaslerCameraNode camera2("s60_n_cam_50_k", "0030532A9B7D");
		PreprocessingNode pre({{"s60_n_cam_16_k", {1200, 1920, cv::ColorConversionCodes::COLOR_BayerBG2BGR}}, {"s60_n_cam_50_k", {1200, 1920, cv::ColorConversionCodes::COLOR_BayerBG2BGR}}});
		ImageStreamRTSP transmitter1([](ImageData const &data) { return data.source == "s60_n_cam_16_k"; });
		ImageStreamRTSP transmitter2([](ImageData const &data) { return data.source == "s60_n_cam_50_k"; });

		camera1 += pre;
		camera2 += pre;
		pre += transmitter1;
		pre += transmitter2;

		camera1();
		camera2();
		pre();
		transmitter1();
		transmitter2();

		std::this_thread::sleep_for(200s);

		clean_up();
	} catch (...) {
		clean_up();
	}
}