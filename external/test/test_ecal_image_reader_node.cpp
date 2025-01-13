#include <EcalReaderNode.h>

#include <stdexcept>

#include "BaslerCamerasNode.h"
#include "EcalImageDriverNode.h"
#include "EcalReaderNode.h"
#include "ImageVisualizationNode.h"
#include "RawDataCamerasSimulatorNode.h"
#include "common_exception.h"

void signal_handler(int signal) {
	common::println("[signal_handler]: got signal '", signal, "'!");

	common::print("Finalize eCAL...");
	eCAL::Finalize();
	common::println("done!");

	std::_Exit(signal);
}

int main(int argc, char* argv[]) {
	gtk_init(&argc, &argv);

	std::signal(SIGINT, signal_handler);
	std::signal(SIGTERM, signal_handler);

	common::print("Initialize eCAL...");
	eCAL::Initialize(argc, argv, "ITS2");
	common::println("done!");

	EcalReaderNode read({{"s110_s_cam_8", {"rgb_s110_s_cam_8_pload_0"}}});
	ImageVisualizationNode img;

	read.synchronously_connect(img);

	auto ecal_read_thread = read();

	for (auto timestamp = std::chrono::system_clock::now() + 10s; std::chrono::system_clock::now() < timestamp; std::this_thread::yield()) g_main_context_iteration(NULL, true);
}