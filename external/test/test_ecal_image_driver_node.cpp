#include <EcalReaderNode.h>

#include <stdexcept>

#include "BaslerCamerasNode.h"
#include "EcalImageDriverNode.h"
#include "EcalReaderNode.h"
#include "RawDataCamerasSimulatorNode.h"
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
		common::print("Initialize Pylon...");
		Pylon::PylonInitialize();
		common::println("done!");

		common::print("Initialize eCAL...");
		eCAL::Initialize(argc, argv, "ITS");
		common::println("done!");

		// BaslerCamerasNode cameras({{"s110_s_cam_8", {"0030532A9B7F"}}, {"s110_o_cam_8", {"003053305C72"}}, {"s110_n_cam_8", {"003053305C75"}}, {"s110_w_cam_8", {"003053380639"}}});
		RawDataCamerasSimulatorNode cameras = make_raw_data_cameras_simulator_node_arrived_recorded1({{"s110_s_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "s110_cams_raw" / "s110_s_cam_8"}});
		EcalImageDriverNode ecal_driver({{"s110_n_cam_8", {{1400.3096617691212, 0., 967.7899705163408, 0., 1403.041082755918, 581.7195041357244, 0., 0., 1.},
		                                                      {-0.17018194636847647, 0.12138270789030073, -0.00011663550730431874, -0.0023506235533554587, -0.030445936493878178}, 1920, 1200}},
		    {"s110_w_cam_8", {{1400.3096617691212, 0., 967.7899705163408, 0., 1403.041082755918, 581.7195041357244, 0., 0., 1.},
		                         {-0.17018194636847647, 0.12138270789030073, -0.00011663550730431874, -0.0023506235533554587, -0.030445936493878178}, 1920, 1200}},
		    {"s110_s_cam_8", {{1400.3096617691212, 0., 967.7899705163408, 0., 1403.041082755918, 581.7195041357244, 0., 0., 1.},
		                         {-0.17018194636847647, 0.12138270789030073, -0.00011663550730431874, -0.0023506235533554587, -0.030445936493878178}, 1920, 1200}},
		    {"s110_o_cam_8", {{1400.3096617691212, 0., 967.7899705163408, 0., 1403.041082755918, 581.7195041357244, 0., 0., 1.},
		                         {-0.17018194636847647, 0.12138270789030073, -0.00011663550730431874, -0.0023506235533554587, -0.030445936493878178}, 1920, 1200}}});

		// BaslerCamerasNode cameras({{"s60_n_cam_16_k", {"00305338063B"}}, {"s60_n_cam_50_k", {"0030532A9B7D"}}});
		// EcalImageDriverNode ecal_driver({{"s60_n_cam_16_k", {{2786.9129510685484, 0., 1017.8812020705767, 0., 2791.8092099418395, 580.5351320595202, 0., 0., 1.},
		//                                                         {-0.21675155648951847, 0.052494576884161384, -0.0017914057577082473, -0.0004466871752013924, 1.3218846850012242}, 1200, 1920}},
		//     {"s60_n_cam_50_k", {{15331.373201139475, 0., 958.6057656556126, 0., 15345.40269814499, 598.3364247894727, 0., 0., 1.}, {3.427766748989152, -408.17098568268386, -0.0012302663776519998, -0.032825209115965065,
		//     -1.1269772923398047},
		//                            1200, 1920}}});

		cameras += ecal_driver;

		cameras();
		ecal_driver();

		std::this_thread::sleep_for(100s);

		clean_up();
	} catch (...) {
		clean_up();
	}
}