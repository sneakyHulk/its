#include <chrono>

#include "ImageDownscalingNode.h"
#include "ImagePreprocessingNode.h"
#include "ImageUndistortionNode.h"
#include "ImageVisualizationNode.h"
#include "ProcessorSynchronousPair.h"
#include "RawDataCamerasSimulatorNode.h"
#include "UndistortDetectionsNode.h"
#include "YoloNode.h"
#include "config.h"

using namespace std::chrono_literals;

class Detection2DVisualization : public ProcessorSynchronousPair<ImageData, Detections2D, ImageData> {
   public:
	Detection2DVisualization()
	    : ProcessorSynchronousPair<ImageData, Detections2D, ImageData>([](ImageData const& data1, Detections2D const& data2) {
		      if (data1.source != data2.source) return false;
		      if (data1.timestamp != data2.timestamp) return false;

		      return true;
	      }) {}

	ImageData process(ImageData const& data1, Detections2D const& data2) final {
		for (auto const object : data2.objects) {
			cv::rectangle(data1.image, cv::Point2d(object.bbox.left, object.bbox.top), cv::Point2d(object.bbox.right, object.bbox.bottom), cv::Scalar_<int>(0, 0, 255), 5);
		}

		return data1;
	}
};

int main(int argc, char** argv) {
	gtk_init(&argc, &argv);

	RawDataCamerasSimulatorNode raw_cams = make_raw_data_cameras_simulator_node_arrived_recorded1({{"s110_s_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "s110_cams_raw" / "s110_s_cam_8"}});
	ImagePreprocessingNode pre({{"s110_n_cam_8", {1200, 1920, cv::ColorConversionCodes::COLOR_BayerBG2BGR}}, {"s110_w_cam_8", {1200, 1920, cv::ColorConversionCodes::COLOR_BayerBG2BGR}},
	    {"s110_s_cam_8", {1200, 1920, cv::ColorConversionCodes::COLOR_BayerBG2BGR}}, {"s110_o_cam_8", {1200, 1920, cv::ColorConversionCodes::COLOR_BayerBG2BGR}}});
	ImageDownscalingNode<480, 640> down;
	YoloNode<480, 640> yolo({{"s110_n_cam_8", {1200, 1920}}, {"s110_s_cam_8", {1200, 1920}}, {"s110_o_cam_8", {1200, 1920}}, {"s110_w_cam_8", {1200, 1920}}});
	UndistortDetectionsNode undistort({{"s110_n_cam_8", {config::intrinsic_matrix_s110_n_cam_8, config::distortion_values_s110_n_cam_8, config::optimal_camera_matrix_s110_n_cam_8}},
	    {"s110_s_cam_8", {config::intrinsic_matrix_s110_s_cam_8, config::distortion_values_s110_s_cam_8, config::optimal_camera_matrix_s110_s_cam_8}},
	    {"s110_w_cam_8", {config::intrinsic_matrix_s110_w_cam_8, config::distortion_values_s110_w_cam_8, config::optimal_camera_matrix_s110_w_cam_8}},
	    {"s110_o_cam_8", {config::intrinsic_matrix_s110_o_cam_8, config::distortion_values_s110_o_cam_8, config::optimal_camera_matrix_s110_o_cam_8}}});

	ImageUndistortionNode img_undistort(
	    {{"s110_n_cam_8", {config::intrinsic_matrix_s110_n_cam_8, config::distortion_values_s110_n_cam_8, config::optimal_camera_matrix_s110_n_cam_8, config::undistortion_map1_s110_n_cam_8, config::undistortion_map2_s110_n_cam_8}},
	        {"s110_s_cam_8", {config::intrinsic_matrix_s110_s_cam_8, config::distortion_values_s110_s_cam_8, config::optimal_camera_matrix_s110_s_cam_8, config::undistortion_map1_s110_s_cam_8, config::undistortion_map2_s110_s_cam_8}},
	        {"s110_w_cam_8", {config::intrinsic_matrix_s110_w_cam_8, config::distortion_values_s110_w_cam_8, config::optimal_camera_matrix_s110_w_cam_8, config::undistortion_map1_s110_w_cam_8, config::undistortion_map2_s110_w_cam_8}},
	        {"s110_o_cam_8", {config::intrinsic_matrix_s110_o_cam_8, config::distortion_values_s110_o_cam_8, config::optimal_camera_matrix_s110_o_cam_8, config::undistortion_map1_s110_o_cam_8, config::undistortion_map2_s110_o_cam_8}}});
	Detection2DVisualization detvis;
	ImageVisualizationNode img([](ImageData const& data) { return data.source == "s110_s_cam_8"; });

	raw_cams.asynchronously_connect(pre);
	pre.synchronously_connect(down).asynchronously_connect(yolo);

	pre.synchronously_connect(img_undistort).synchronously_connect(detvis);
	yolo.synchronously_connect(undistort);
	undistort.synchronously_connect(detvis);
	detvis.synchronously_connect(img);

	auto raw_cams_thread = raw_cams();
	auto pre_thread = pre();
	auto yolo_thread = yolo();

	for (auto timestamp = std::chrono::system_clock::now() + 20s; std::chrono::system_clock::now() < timestamp; std::this_thread::yield()) g_main_context_iteration(NULL, true);
}