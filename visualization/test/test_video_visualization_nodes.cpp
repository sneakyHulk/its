#include <chrono>

#include "CamerasSimulatorNode.h"
#include "VideoVisualizationNode.h"

using namespace std::chrono_literals;

int main(int argc, char* argv[]) {
	CamerasSimulatorNode cameras = make_cameras_simulator_node_arrived1({{"s110_n_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "s110_cams" / "s110_n_cam_8" / "s110_n_cam_8_images_distorted"}});
	VideoVisualizationNode vid;

	cameras.asynchronously_connect(vid);

	auto cameras_thread = cameras();
	auto video_thread = vid();

	std::this_thread::sleep_for(100s);
}