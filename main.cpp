#include <thread>

#include "Config.h"
#include "camera_simulator_node.h"
#include "data_communication_node.h"
#include "image_communication_node.h"
#include "tracking_node.h"
#include "tracking_visualization_node.h"
#include "transformation_node.h"
#include "undistort_node.h"
#include "visualization_node.h"
#include "yolo_node.h"

#define undistort_enable 1
#define track_vis_enable 0
#define stream_data_enable 0
#define stream_video_enable 0

int main() {
	Config config = make_config();

	CameraSimulator cam_n("s110_n_cam_8");
	CameraSimulator cam_o("s110_o_cam_8");
	CameraSimulator cam_s("s110_s_cam_8");
	CameraSimulator cam_w("s110_w_cam_8");
#if stream_video_enable == 1
	ImageStreamRTSP video_stream_w;
	ImageStreamRTSP video_stream_n;
	ImageStreamRTSP video_stream_o;
	ImageStreamRTSP video_stream_s;
#endif
	Yolo yolo;
#if undistort_enable == 1
	UndistortDetections undistort(config);
#endif
	SortTracking track(config);
#if track_vis_enable == 1
	ImageTrackingVisualization2 track_vis;
#endif
	ImageTrackingTransformation trans(config);
#if stream_data_enable == 1
	DataStreamMQTT data_stream;
#endif
	Visualization2D vis(config);

#if stream_video_enable == 1
	cam_w += video_stream_w;
	cam_n += video_stream_n;
	cam_o += video_stream_o;
	cam_s += video_stream_s;
#endif

	cam_n += yolo;
	cam_o += yolo;
	cam_s += yolo;
	cam_w += yolo;

#if undistort_enable == 1
	yolo += undistort;
	undistort += track;
#else
	yolo += track;
#endif

#if track_vis_enable == 1
	cam_w += track_vis;
	track += track_vis;
#endif

	track += trans;

#if stream_data_enable == 1
	trans += data_stream;
#endif

	trans += vis;

	// std::thread vis_thread(&Visualization2D::operator(), &vis);

	std::thread cam_n_thread(&CameraSimulator::operator(), &cam_n);
	std::thread cam_o_thread(&CameraSimulator::operator(), &cam_o);
	std::thread cam_s_thread(&CameraSimulator::operator(), &cam_s);
	std::thread cam_w_thread(&CameraSimulator::operator(), &cam_w);

	std::thread yolo_thread(&Yolo::operator(), &yolo);
#if undistort_enable == 1
	std::thread undistort_thread(&UndistortDetections::operator(), &undistort);
#endif
	std::thread track_thread(&SortTracking::operator(), &track);
#if track_vis_enable == 1
	std::thread track_vis_thread = std::thread(&ImageTrackingVisualization2::operator(), &track_vis);
#endif
	std::thread trans_thread(&ImageTrackingTransformation::operator(), &trans);
#if stream_data_enable == 1
	std::thread data_stream_thread(&DataStreamMQTT::operator(), &data_stream);
#endif
#if stream_video_enable == 1
	std::thread video_stream_w_thread(&ImageStreamRTSP::operator(), &video_stream_w);
	std::thread video_stream_n_thread(&ImageStreamRTSP::operator(), &video_stream_n);
	std::thread video_stream_o_thread(&ImageStreamRTSP::operator(), &video_stream_o);
	std::thread video_stream_s_thread(&ImageStreamRTSP::operator(), &video_stream_s);

	ImageStreamRTSP::run_loop();
#else
	std::this_thread::sleep_for(378432000s);
#endif
}