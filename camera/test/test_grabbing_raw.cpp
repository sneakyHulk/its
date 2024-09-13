#include <csignal>

#include "camera_node.h"
#include "common_output.h"

int main() {
	BaslerCameras cams;
	BaslerSaveRAW saving(cams);

	cams += saving;

	std::thread vid_s_thread(&BaslerCameras::operator(), &cams);
	saving();

	return 0;
}