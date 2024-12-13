#include <csignal>
#include <thread>

#include "BaslerCamerasNode.h"
#include "CamerasSimulatorNode.h"
#include "ImagePreprocessingNode.h"
#include "ReceivingImageNode.h"
#include "StreamingImageNode.h"

// gst-launch-1.0 rtspsrc location=rtsp://80.155.138.138:2346/s60_n_cam_16_k latency=100 ! queue ! rtph264depay ! h264parse ! avdec_h264 ! videoconvert ! autovideosink
int main(int argc, char **argv) {
	gst_init(&argc, &argv);

	CamerasSimulatorNode cameras = make_cameras_simulator_node_arrived1({{"s110_n_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "s110_cams" / "s110_n_cam_8" / "s110_n_cam_8_images"}});
	StreamingImageNode transmitter;
	cameras.asynchronously_connect(transmitter);

	cameras();
	transmitter();

	for (;; std::this_thread::yield()) g_main_context_iteration(NULL, true);
}