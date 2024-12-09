#include "CamerasSimulatorNode.h"
#include "video_node.h"

int main(int argc, char* argv[]) {
	CamerasSimulatorNode cameras = make_cameras_simulator_node_arrived1({{"s110_n_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "s110_cams" / "s110_n_cam_8" / "s110_n_cam_8_images_distorted"}});
	VideoVisualization vid;

	cameras.asynchronously_connect(vid);

	cameras();
	vid();
}