#include <csignal>
#include <thread>

#include "CamerasSimulatorNode.h"
#include "ReceivingImageNode.h"
#include "StreamingImageNode.h"

int main(int argc, char **argv) {
	gst_init(&argc, &argv);

	CamerasSimulatorNode cams = make_cameras_simulator_node_tumtraf({{"s110_n_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "tumtraf_v2x_cooperative_perception_dataset" / "train" / "images" / "s110_camera_basler_north_8mm"},
	    {"s110_n_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "tumtraf_v2x_cooperative_perception_dataset" / "test" / "images" / "s110_camera_basler_north_8mm"},
	    {"s110_n_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "tumtraf_v2x_cooperative_perception_dataset" / "val" / "images" / "s110_camera_basler_north_8mm"}});

	StreamingImageNode transmitter;
	cams.asynchronously_connect(transmitter);

	auto cameras_thread = cams();
	auto transmitter_thread = transmitter();

	for (auto timestamp = std::chrono::system_clock::now() + 40s; std::chrono::system_clock::now() < timestamp; std::this_thread::yield()) g_main_context_iteration(NULL, false);
}