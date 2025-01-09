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

	std::jthread const *receiver_thread_ptr;

	ReceivingImageNode receiver(receiver_thread_ptr);
	ImageVisualizationNode vis;

	receiver.synchronously_connect(vis);

	receiver_thread_ptr = new std::jthread(receiver());

	for (auto timestamp = std::chrono::system_clock::now() + 10s; std::chrono::system_clock::now() < timestamp; std::this_thread::yield()) {
		common::println_loc("lol");
		g_main_context_iteration(NULL, false);
	}

	delete receiver_thread_ptr;
}