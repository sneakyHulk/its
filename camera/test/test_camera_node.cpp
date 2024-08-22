#include <boost/circular_buffer.hpp>
#include <filesystem>

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

	{
		Camera cam_s("0030532A9B7F", "s110_s_cam_8");
		// Camera cam_o("003053305C72", "s110_o_cam_8");
		// Camera cam_n("003053305C75", "s110_n_cam_8");
		// Camera cam_w("003053380639", "s110_w_cam_8");

		VideoVisualization vid_s;
		// VideoVisualization vid_o;
		// VideoVisualization vid_n;
		// VideoVisualization vid_w;

		cam_s += vid_s;
		// cam_o += vid_o;
		// cam_n += vid_n;
		// cam_w += vid_w;

		std::thread cam_s_thread(&Camera::operator(), &cam_s);
		// std::thread cam_o_thread(&Camera::operator(), &cam_o);
		// std::thread cam_n_thread(&Camera::operator(), &cam_n);
		// std::thread cam_w_thread(&Camera::operator(), &cam_w);

		std::thread vid_s_thread(&VideoVisualization::operator(), &vid_s);
		// std::thread vid_o_thread(&VideoVisualization::operator(), &vid_o);
		// std::thread vid_n_thread(&VideoVisualization::operator(), &vid_n);
		// std::thread vid_w_thread(&VideoVisualization::operator(), &vid_w);

		std::this_thread::sleep_for(100000s);
	}
}