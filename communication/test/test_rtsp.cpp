#include <thread>

#include "camera_simulator_node.h"
#include "image_communication_node.h"

int main(int argc, char **argv) {
	CameraSimulator cam_n("s110_n_cam_8");
	CameraSimulator cam_o("s110_o_cam_8");
	ImageStreamRTSP transmitter;
	ImageStreamRTSP transmitter2;

	cam_n += transmitter;
	cam_o += transmitter2;

	std::thread cam_n_thread(&CameraSimulator::operator(), &cam_n);
	std::thread cam_o_thread(&CameraSimulator::operator(), &cam_o);
	std::thread transmitter_thread(&ImageStreamRTSP::operator(), &transmitter);
	std::thread transmitter2_thread(&ImageStreamRTSP::operator(), &transmitter2);

	ImageStreamRTSP::run_loop();
}