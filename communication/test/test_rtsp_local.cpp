#include <csignal>
#include <thread>

#include "BaslerCamerasNode.h"
#include "CamerasSimulatorNode.h"
#include "PreprocessingNode.h"
#include "StreamingImageNode.h"
#include "image_communication_node.h"

std::function<void()> clean_up;

void signal_handler(int signal) {
	common::println("[signal_handler]: got signal '", signal, "'!");
	clean_up();
}

// gst-launch-1.0 rtspsrc location=rtsp://80.155.138.138:2346/s60_n_cam_16_k latency=100 ! queue ! rtph264depay ! h264parse ! avdec_h264 ! videoconvert ! autovideosink
int main(int argc, char **argv) {
	gst_init(&argc, &argv);

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

		//CamerasSimulatorNode cameras = make_cameras_simulator_node_arrived1({{"s110_n_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "s110_cams" / "s110_n_cam_8" / "s110_n_cam_8_images_distorted"}});
		StreamingImageNode transmitter;

		//cameras.asynchronously_connect(transmitter);

		//cameras();
		transmitter();

		std::this_thread::sleep_for(200s);

		clean_up();
	} catch (...) {
		clean_up();
	}
}