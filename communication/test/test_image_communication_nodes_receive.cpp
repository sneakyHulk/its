#include <csignal>
#include <opencv2/opencv.hpp>
#include <thread>

#include "ImageVisualizationNode.h"
#include "ReceivingImageNode.h"
#include "common_output.h"

using namespace std::chrono_literals;

int main(int argc, char **argv) {
	gst_init(&argc, &argv);
	gtk_init(&argc, &argv);

	ReceivingImageNode receiver;
	ImageVisualizationNode vis;

	receiver.synchronously_connect(vis);

	auto receiver_thread = receiver();

	for (auto timestamp = std::chrono::system_clock::now() + 40s; std::chrono::system_clock::now() < timestamp; std::this_thread::yield()) g_main_context_iteration(NULL, false);
}