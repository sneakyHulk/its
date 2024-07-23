#include <thread>

#include "Config.h"
#include "camera_simulator_node.h"
#include "common_output.h"
#include "data_communication_node.h"
#include "tracking_node.h"
#include "tracking_visualization_node.h"
#include "transformation_node.h"
#include "undistort_node.h"
#include "visualization_node.h"
#include "yolo_node.h"

constexpr bool undistort_enable = true;
constexpr bool track_vis_enable = false;

int main() {
	Config config = make_config();

	CameraSimulator cam_n("s110_n_cam_8");
	CameraSimulator cam_o("s110_o_cam_8");
	CameraSimulator cam_s("s110_s_cam_8");
	CameraSimulator cam_w("s110_w_cam_8");
	Yolo yolo;

	SortTracking track(config);
	ImageTrackingTransformation trans(config);
	Visualization2D vis(config);

	cam_n += yolo;
	cam_o += yolo;
	cam_s += yolo;
	cam_w += yolo;

	UndistortDetections undistort(config);
	std::thread undistort_thread;

	if constexpr (undistort_enable) {
		yolo += undistort;
		undistort += track;
		undistort_thread = std::thread(&UndistortDetections::operator(), &undistort);
	} else {
		yolo += track;
	}

	ImageTrackingVisualizationHelper track_vis_helper;
	ImageTrackingVisualization track_vis(track_vis_helper, "s110_w_cam_8");
	std::thread track_vis_helper_thread;
	std::thread track_vis_thread;

	if constexpr (track_vis_enable) {
		cam_w += track_vis_helper;
		track += track_vis;

		// if undistort is enabled, it lays not perfectly on the objects!!
		track_vis_helper_thread = std::thread(&ImageTrackingVisualizationHelper::operator(), &track_vis_helper);
		track_vis_thread = std::thread(&ImageTrackingVisualization::operator(), &track_vis);
	}

	DataStreamMQTT data_stream;

	track += trans;
	trans += vis;
	trans += data_stream;

	std::thread cam_n_thread(&CameraSimulator::operator(), &cam_n);
	std::thread cam_o_thread(&CameraSimulator::operator(), &cam_o);
	std::thread cam_s_thread(&CameraSimulator::operator(), &cam_s);
	std::thread cam_w_thread(&CameraSimulator::operator(), &cam_w);
	std::thread yolo_thread(&Yolo::operator(), &yolo);

	std::thread track_thread(&SortTracking::operator(), &track);
	std::thread trans_thread(&ImageTrackingTransformation::operator(), &trans);
	std::thread data_stream_thread(&DataStreamMQTT::operator(), &data_stream);

	vis();
}