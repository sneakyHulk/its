#include <gtk/gtk.h>

#include <chrono>
#include <filesystem>

#include "CamerasSimulatorNode.h"
#include "ImageVisualizationNode.h"

using namespace std::chrono_literals;

int main(int argc, char* argv[]) {
	gtk_init(&argc, &argv);

	CamerasSimulatorNode cameras = make_cameras_simulator_node_arrived1({{"s110_n_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "s110_cams" / "s110_n_cam_8" / "s110_n_cam_8_images"}});
	ImageVisualizationNode vis;
	ImageVisualizationNode vis2;

	cameras.synchronously_connect(vis);
	cameras.synchronously_connect(vis2);

	cameras();

	for (;; std::this_thread::yield()) g_main_context_iteration(NULL, true);
}