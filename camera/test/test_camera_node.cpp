#include "camera_node.h"
#include "video_node.h"


int main(int argc, char* argv[]) {
	Camera cam;

	VideoVisualization vid;

	std::thread cam_n_thread(&Camera::operator(), &cam);
	vid();
}