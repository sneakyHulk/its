#include <stdexcept>

#include "BaslerCamerasNode.h"
#include "EcalImageDriverNode.h"
#include "common_exception.h"

void aggregate_signal_handler(int signal) {
	common::print("Terminate Pylon...");
	Pylon::PylonTerminate();
	common::println("terminated!");

	common::print("Finalize eCAL...");
	eCAL::Finalize();
	common::println("finalized!");
	std::_Exit(signal);
}
int main(int argc, char* argv[]) {
	std::signal(SIGINT, aggregate_signal_handler);
	std::signal(SIGTERM, aggregate_signal_handler);
	try {
		common::print("Initialize Pylon...");
		Pylon::PylonInitialize();
		common::println("initialzed!");

		common::print("Initialize eCAL...");
		eCAL::Initialize(argc, argv, "ITS");
		common::println("initialzed!");

		BaslerCamerasNode cameras({{"s110_s_cam_8", {"0030532A9B7F"}}, {"s110_o_cam_8", {"003053305C72"}}, {"s110_n_cam_8", {"003053305C75"}}, {"s110_w_cam_8", {"003053380639"}}});
		EcalImageDriverNode ecal_driver({{"s110_n_cam_8", {{1400.3096617691212, 0., 967.7899705163408, 0., 1403.041082755918, 581.7195041357244, 0., 0., 1.},
		                                                      {-0.17018194636847647, 0.12138270789030073, -0.00011663550730431874, -0.0023506235533554587, -0.030445936493878178}, 1200, 1920}},
		    {"s110_w_cam_8", {{1400.3096617691212, 0., 967.7899705163408, 0., 1403.041082755918, 581.7195041357244, 0., 0., 1.},
		                         {-0.17018194636847647, 0.12138270789030073, -0.00011663550730431874, -0.0023506235533554587, -0.030445936493878178}, 1200, 1920}},
		    {"s110_s_cam_8", {{1400.3096617691212, 0., 967.7899705163408, 0., 1403.041082755918, 581.7195041357244, 0., 0., 1.},
		                         {-0.17018194636847647, 0.12138270789030073, -0.00011663550730431874, -0.0023506235533554587, -0.030445936493878178}, 1200, 1920}},
		    {"s110_o_cam_8", {{1400.3096617691212, 0., 967.7899705163408, 0., 1403.041082755918, 581.7195041357244, 0., 0., 1.},
		                         {-0.17018194636847647, 0.12138270789030073, -0.00011663550730431874, -0.0023506235533554587, -0.030445936493878178}, 1200, 1920}}});

		cameras += ecal_driver;

		std::thread cameras_thread(&BaslerCamerasNode::operator(), &cameras);

		std::this_thread::sleep_for(10s);
	} catch (...) {
		common::print("Terminate Pylon...");
		Pylon::PylonTerminate();
		common::println("terminated!");

		common::print("Finalize eCAL...");
		eCAL::Finalize();
		common::println("finalized!");

		rethrow_exception(std::current_exception());
	}
}