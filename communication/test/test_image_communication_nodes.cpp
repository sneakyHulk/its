#include <csignal>
#include <thread>

#include "CamerasSimulatorNode.h"
#include "ReceivingImageNode.h"
#include "StreamingImageNode.h"

int main(int argc, char **argv) {
	gst_init(&argc, &argv);

	CamerasSimulatorNode cameras = make_cameras_simulator_node_arrived1({{"s110_n_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "s110_cams" / "s110_n_cam_8" / "s110_n_cam_8_images"}});
	StreamingImageNode transmitter;
	cameras.asynchronously_connect(transmitter);

	auto cameras_thread = cameras();
	auto transmitter_thread = transmitter();

	for (auto timestamp = std::chrono::system_clock::now() + 40s; std::chrono::system_clock::now() < timestamp; std::this_thread::yield()) g_main_context_iteration(NULL, false);
}