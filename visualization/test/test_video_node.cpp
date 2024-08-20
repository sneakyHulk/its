#include "camera_simulator_node.h"
#include "video_node.h"

int main(int argc, char* argv[]) {
	CameraSimulator cam_n("s110_n_cam_8");
	VideoVisualization vid;

	cam_n += vid;

	std::thread cam_n_thread(&CameraSimulator::operator(), &cam_n);
	vid();
}