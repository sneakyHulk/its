#include <csignal>

#include "../../thirdparty/common/include/common_output.h"
#include "camera_node.h"

int main() {
	BaslerCameras cams;
	BaslerSaveRAW saving(cams);

	cams += saving;

	std::thread vid_s_thread(&BaslerCameras::operator(), &cams);
	saving();

	return 0;
}