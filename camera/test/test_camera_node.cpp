#include <boost/circular_buffer.hpp>

#include "camera_node.h"
#include "video_node.h"

class PylonRAII {
   public:
	PylonRAII() { Pylon::PylonInitialize(); }
	~PylonRAII() { Pylon::PylonTerminate(); }
};

int main(int argc, char* argv[]) {
	// Before using any pylon methods, the pylon runtime must be initialized.
	PylonRAII pylon_raii;

	Camera cam;

	VideoVisualization vid;

	std::thread cam_n_thread(&Camera::operator(), &cam);
	vid();
}