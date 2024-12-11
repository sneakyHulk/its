#include <csignal>
#include <opencv2/opencv.hpp>
#include <thread>

#include "ImageVisualizationNode.h"
#include "ReceivingImageNode.h"

using namespace std::chrono_literals;

class AfterReturnMessage {
   public:
	~AfterReturnMessage() { common::println_loc("END"); }
};

// gst-launch-1.0 rtspsrc location=rtsp://80.155.138.138:2346/s60_n_cam_16_k latency=100 ! queue ! rtph264depay ! h264parse ! avdec_h264 ! videoconvert ! autovideosink
int main(int argc, char **argv) {
	gst_init(&argc, &argv);
	auto app = Gtk::Application::create(std::string("sneakyHulk.its.") + std::filesystem::path(argv[0]).stem().string());


	ReceivingImageNode receiver;
	ImageVisualizationNode vis(app);

	receiver.synchronously_connect(vis);

	receiver();

	app->run();

	for (;; std::this_thread::yield()) g_main_context_iteration(NULL, true);
}