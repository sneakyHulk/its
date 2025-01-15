#include <chrono>

#include "CamerasSimulatorNode.h"
#include "ImageDownscalingNode.h"
#include "ImagePreprocessingNode.h"
#include "ImageUndistortionNode.h"
#include "ImageVisualizationNode.h"
#include "ProcessorSynchronousPair.h"
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

	// With this, images there is no undistortion needed but for testing purposes images are needed...
	CamerasSimulatorNode cams = make_cameras_simulator_node_tumtraf({{"s110_n_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "tumtraf_v2x_cooperative_perception_dataset" / "train" / "images" / "s110_camera_basler_north_8mm"},
	    {"s110_o_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "tumtraf_v2x_cooperative_perception_dataset" / "train" / "images" / "s110_camera_basler_east_8mm"},
	    {"s110_w_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "tumtraf_v2x_cooperative_perception_dataset" / "train" / "images" / "s110_camera_basler_south1_8mm"},
	    {"s110_s_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "tumtraf_v2x_cooperative_perception_dataset" / "train" / "images" / "s110_camera_basler_south2_8mm"},
	    {"s110_n_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "tumtraf_v2x_cooperative_perception_dataset" / "test" / "images" / "s110_camera_basler_north_8mm"},
	    {"s110_o_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "tumtraf_v2x_cooperative_perception_dataset" / "test" / "images" / "s110_camera_basler_east_8mm"},
	    {"s110_w_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "tumtraf_v2x_cooperative_perception_dataset" / "test" / "images" / "s110_camera_basler_south1_8mm"},
	    {"s110_s_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "tumtraf_v2x_cooperative_perception_dataset" / "test" / "images" / "s110_camera_basler_south2_8mm"},
	    {"s110_n_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "tumtraf_v2x_cooperative_perception_dataset" / "val" / "images" / "s110_camera_basler_north_8mm"},
	    {"s110_o_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "tumtraf_v2x_cooperative_perception_dataset" / "val" / "images" / "s110_camera_basler_east_8mm"},
	    {"s110_w_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "tumtraf_v2x_cooperative_perception_dataset" / "val" / "images" / "s110_camera_basler_south1_8mm"},
	    {"s110_s_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "tumtraf_v2x_cooperative_perception_dataset" / "val" / "images" / "s110_camera_basler_south2_8mm"}});
	ImageDownscalingNode<480, 640> down;
	YoloNode<480, 640> yolo(
	    {{"s110_n_cam_8", {1200, 1920}}, {"s110_s_cam_8", {1200, 1920}}, {"s110_o_cam_8", {1200, 1920}}, {"s110_w_cam_8", {1200, 1920}}}, std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "yolo" / "480x640" / "yolo11m.torchscript");
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

	cams.asynchronously_connect(down);
	down.asynchronously_connect(yolo);

	cams.synchronously_connect(img_undistort).synchronously_connect(detvis);
	yolo.synchronously_connect(undistort);
	undistort.synchronously_connect(detvis);
	detvis.synchronously_connect(img);

	auto cams_thread = cams();
	auto down_thread = down();
	auto yolo_thread = yolo();

	for (auto timestamp = std::chrono::system_clock::now() + 20s; std::chrono::system_clock::now() < timestamp; std::this_thread::yield()) g_main_context_iteration(NULL, true);
}