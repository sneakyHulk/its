#include <chrono>

#include "BirdEyeVisualizationNode.h"
#include "Downscaling.h"
#include "DrawingUtils.h"
#include "ImageDetectionVisualizationNode.h"
#include "ImageTrackerNode.h"
#include "ImageVisualizationNode.h"
#include "Preprocessing.h"
#include "RawDataCameraSimulator.h"
#include "TrackToTrackFusion.h"
#include "TrackingVisualizationNode.h"
#include "UndistortionNode.h"
#include "YoloNode.h"
#include "data_communication_node.h"
#include "image_communication_node.h"
#include "thread"
#include "tracking_node.h"

using namespace std::chrono_literals;

int main() {
	ImageStreamRTSP<false> image_stream;
	BayerBG8CameraSimulator cams({{"s110_s_cam_8",
	    std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "s110_cams" /
	        "s110_s_cam_8"}});  //, {"s110_n_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "s110_cams" / "s110_n_cam_8"},
	                            // {"s110_w_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "s110_cams" / "s110_w_cam_8"}, {"s110_o_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "s110_cams" / "s110_o_cam_8"}});
	BayerBG8Preprocessing preprocessing({{"s110_n_cam_8", {1200, 1920}}, {"s110_w_cam_8", {1200, 1920}}, {"s110_s_cam_8", {1200, 1920}}, {"s110_o_cam_8", {1200, 1920}}});
	// DownscalingNode<480, 640> down;

	cams += preprocessing;
	// preprocessing += down;
	preprocessing += image_stream;
	// down += image_stream;

	std::thread cams_thread(&BayerBG8CameraSimulator::operator(), &cams);
	std::thread preprocessing_thread(&BayerBG8Preprocessing::operator(), &preprocessing);
	// std::thread down_thread(&DownscalingNode<480, 640>::operator(), &down);

	std::thread image_stream_thread(&decltype(image_stream)::operator(), &image_stream);

	std::thread image_stream_loop_thread(decltype(image_stream)::run_loop);

	std::this_thread::sleep_for(1min);
}