#include <thread>

#include "camera_simulator_node.h"
#include "common_output.h"
#include "tracking_node.h"
#include "Config.h"
#include "transformation_node.h"
#include "undistort_node.h"
#include "tracking_visualization_node.h"
#include "visualization_node.h"
#include "yolo_node.h"

int main() {
	Config config = make_config();

	CameraSimulator cam_n("s110_n_cam_8");
	CameraSimulator cam_o("s110_o_cam_8");
	CameraSimulator cam_s("s110_s_cam_8");
	CameraSimulator cam_w("s110_w_cam_8");
	Yolo yolo;
	UndistortDetections undistort(config);
	SortTracking track(config);
	ImageTrackingTransformation trans(config);
	Visualization2D vis(config);
	ImageTrackingVisualizationHelper track_vis_helper;
	ImageTrackingVisualization track_vis(track_vis_helper, "s110_s_cam_8");

	GlobalImageTracking glob_track(config);
	GlobalTrackingTransformation glob_trans(config);

	cam_n += yolo;
	cam_o += yolo;
	cam_s += yolo;
	cam_w += yolo;

	yolo += undistort;

	undistort += glob_track;
	glob_track += glob_trans;
	glob_trans += vis;

	// undistort += track;

	// yolo += track;
	// cam_s += track_vis_helper;
	// track += track_vis;

	// track += trans;

	// trans += vis;

	std::thread cam_n_thread(&CameraSimulator::operator(), &cam_n);
	std::thread cam_o_thread(&CameraSimulator::operator(), &cam_o);
	std::thread cam_s_thread(&CameraSimulator::operator(), &cam_s);
	std::thread cam_w_thread(&CameraSimulator::operator(), &cam_w);
	std::thread yolo_thread(&Yolo::operator(), &yolo);
	std::thread undistort_thread(&UndistortDetections::operator(), &undistort);
	std::thread glob_track_thread(&GlobalImageTracking::operator(), &glob_track);
	std::thread glob_trans_thread(&GlobalTrackingTransformation::operator(), &glob_trans);



	//std::thread track_thread(&SortTracking::operator(), &track);
	//std::thread trans_thread(&ImageTrackingTransformation::operator(), &trans);
	//std::thread track_vis_helper_thread(&ImageTrackingVisualizationHelper::operator(), &track_vis_helper);
	//std::thread track_vis_thread(&ImageTrackingVisualization::operator(), &track_vis);
	vis();
}