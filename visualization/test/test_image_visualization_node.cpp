#include <gtkmm.h>

#include <chrono>
#include <filesystem>

#include "CamerasSimulatorNode.h"
#include "ImageVisualizationNode.h"

using namespace std::chrono_literals;

int main(int argc, char* argv[]) {
	auto app = Gtk::Application::create(std::string("sneakyHulk.its.") + std::filesystem::path(argv[0]).stem().string());

	CamerasSimulatorNode cameras = make_cameras_simulator_node_arrived1({{"s110_n_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "s110_cams" / "s110_n_cam_8" / "s110_n_cam_8_images_distorted"}});
	ImageVisualizationNode vis(app);

	cameras.synchronously_connect(vis);

	cameras();

	return app->run();
}