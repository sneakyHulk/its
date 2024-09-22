#include "BirdEyeVisualizationNode.h"
#include "CameraSimulator.h"
#include "Downscaling.h"
#include "DrawingUtils.h"
#include "ImageTrackerNode.h"
#include "ImageVisualizationNode.h"
#include "Preprocessing.h"
#include "UndistortionNode.h"
#include "YoloNode.h"
#include "thread"
#include "tracking_node.h"
#include "TrackingVisualizationNode.h"
#include "ImageDetectionVisualizationNode.h"

int main() {
	BayerBG8CameraSimulator cams({{"s110_s_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "s110_cams" / "s110_s_cam_8"}}); //, {"s110_n_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "s110_cams" / "s110_n_cam_8"},
	   // {"s110_w_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "s110_cams" / "s110_w_cam_8"}, {"s110_o_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "s110_cams" / "s110_o_cam_8"}});

	BayerBG8Preprocessing preprocessing({{"s110_n_cam_8", {1200, 1920}}, {"s110_w_cam_8", {1200, 1920}}, {"s110_s_cam_8", {1200, 1920}}, {"s110_o_cam_8", {1200, 1920}}});

	// ImageVisualizationNode vis([](ImageData const& data) { return data.source == "s110_s_cam_8"; });
	ImageVisualizationNode vis;
	DownscalingNode<480, 640> down;
	YoloNode<480, 640> yolo;
	UndistortionNode undistort({{"s110_n_cam_8", {{1400.3096617691212, 0., 967.7899705163408, 0., 1403.041082755918, 581.7195041357244, 0., 0., 1.},
	                                                 {-0.17018194636847647, 0.12138270789030073, -0.00011663550730431874, -0.0023506235533554587, -0.030445936493878178}, 1200, 1920}},
	    {"s110_w_cam_8", {{1400.3096617691212, 0., 967.7899705163408, 0., 1403.041082755918, 581.7195041357244, 0., 0., 1.},
	                         {-0.17018194636847647, 0.12138270789030073, -0.00011663550730431874, -0.0023506235533554587, -0.030445936493878178}, 1200, 1920}},
	    {"s110_s_cam_8", {{1400.3096617691212, 0., 967.7899705163408, 0., 1403.041082755918, 581.7195041357244, 0., 0., 1.},
	                         {-0.17018194636847647, 0.12138270789030073, -0.00011663550730431874, -0.0023506235533554587, -0.030445936493878178}, 1200, 1920}},
	    {"s110_o_cam_8", {{1400.3096617691212, 0., 967.7899705163408, 0., 1403.041082755918, 581.7195041357244, 0., 0., 1.},
	                         {-0.17018194636847647, 0.12138270789030073, -0.00011663550730431874, -0.0023506235533554587, -0.030445936493878178}, 1200, 1920}}});

	ImageTrackerNode<> track;

	Config config = make_config();
	cv::Mat map;
	Eigen::Matrix<double, 4, 4> utm_to_image;
	std::tie(map, utm_to_image) = draw_map(std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "visualization" / "2021-07-07_1490_Providentia_Plus_Plus_1_6.xodr", config);
	BirdEyeVisualizationNode vis2d(map, utm_to_image);
	//TrackingVisualizationNode track_vis([](ImageData const& data) { return data.source == "s110_s_cam_8"; });

	ImageDetectionVisualizationNode img_det_vis([](ImageData const& data) { return data.source == "s110_s_cam_8"; });

	cams += preprocessing;
	preprocessing += down;
	down += yolo;
	yolo += undistort;

	//preprocessing += img_det_vis;
	//yolo+= img_det_vis;

	undistort += track;



	//preprocessing += track_vis;
	//track += track_vis;

	// undistort += glob_track;
	// glob_track += vis2d;
	//vis2d += vis;

	std::thread cams_thread(&BayerBG8CameraSimulator::operator(), &cams);
	std::thread preprocessing_thread(&BayerBG8Preprocessing::operator(), &preprocessing);
	std::thread down_thread(&DownscalingNode<480, 640>::operator(), &down);
	std::thread yolo_thread(&YoloNode<480, 640>::operator(), &yolo);
	std::thread undistort_thread(&UndistortionNode::operator(), &undistort);
	std::thread track_thread(&ImageTrackerNode<>::operator(), &track);
	//std::thread track_vis_thread(&TrackingVisualizationNode::operator(), &track_vis);
	//std::thread vis2d_thread(&BirdEyeVisualizationNode::operator(), &vis2d);
	//std::thread img_det_vis_thread(&ImageDetectionVisualizationNode::operator(), &img_det_vis);

	//vis();
	//img_det_vis();

	vis2d();
}