#include <EcalReaderNode.h>

#include <stdexcept>

#include "BaslerCamerasNode.h"
#include "EcalImageDriverNode.h"
#include "EcalReaderNode.h"
#include "ImageVisualizationNode.h"
#include "RawDataCamerasSimulatorNode.h"
#include "common_exception.h"

std::function<void()> clean_up;

void signal_handler(int signal) {
	common::println("[signal_handler]: got signal '", signal, "'!");
	clean_up();
}

int main(int argc, char* argv[]) {
	clean_up = []() {
		common::print("Finalize eCAL...");
		eCAL::Finalize();
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
		common::print("Initialize eCAL...");
		eCAL::Initialize(argc, argv, "ITS2");
		common::println("done!");

		EcalReaderNode read({{"s110_s_cam_8", {"rgb_s110_s_cam_8_pload_0"}}});
		ImageVisualizationNode img;

		read += img;

		read();
		img();

		std::this_thread::sleep_for(100s);

		clean_up();
	} catch (...) {
		clean_up();
	}
}