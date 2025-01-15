#include <chrono>

#include "BirdEyeVisualizationNode.h"
#include "CamerasSimulatorNode.h"
#include "DrawingUtils.h"
#include "ImageDownscalingNode.h"
#include "ImagePreprocessingNode.h"
#include "ImageTrackerNode.h"
#include "ImageVisualizationNode.h"
#include "ProcessorSynchronousPair.h"
#include "TrackToTrackFusion.h"
#include "YoloNode.h"
#include "config.h"

using namespace std::chrono_literals;

class ImageTrackerResultsVisualization : public ProcessorSynchronousPair<ImageData, ImageTrackerResults, ImageData> {
   public:
	ImageTrackerResultsVisualization()
	    : ProcessorSynchronousPair<ImageData, ImageTrackerResults, ImageData>([](ImageData const& data1, ImageTrackerResults const& data2) {
		      if (data1.source != data2.source) return false;
		      if (data1.timestamp != data2.timestamp) return false;

		      return true;
	      }) {}

	ImageData process(ImageData const& data1, ImageTrackerResults const& data2) final {
		for (auto const object : data2.objects) {
			cv::rectangle(data1.image, cv::Point2d(object.state().left, object.state().top), cv::Point2d(object.state().right, object.state().bottom), cv::Scalar_<int>(0, 0, 255), 5);
			cv::putText(data1.image, std::to_string(object.id()), cv::Point2d(object.state().left, object.state().top), cv::FONT_HERSHEY_DUPLEX, 1.0, cv::Scalar_<int>(0, 0, 0), 1);
		}

		return data1;
	}
};

int main(int argc, char** argv) {
	auto const [map, utm_to_image] = draw_map(std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "visualization" / "2021-07-07_1490_Providentia_Plus_Plus_1_6.xodr", 1200, 1920, 5., config::affine_transformation_utm_to_s110_base_north,
	    {{"s110_n_cam_8", {config::projection_matrix_s110_base_north_into_s110_n_cam_8, config::height_s110_n_cam_8, config::width_s110_n_cam_8}},
	        {"s110_o_cam_8", {config::projection_matrix_s110_base_north_into_s110_o_cam_8, config::height_s110_o_cam_8, config::width_s110_o_cam_8}},
	        {"s110_s_cam_8", {config::projection_matrix_s110_base_north_into_s110_s_cam_8, config::height_s110_s_cam_8, config::width_s110_s_cam_8}},
	        {"s110_w_cam_8", {config::projection_matrix_s110_base_north_into_s110_w_cam_8, config::height_s110_w_cam_8, config::width_s110_w_cam_8}}});

	gtk_init(&argc, &argv);

	{
		CamerasSimulatorNode cams =
		    make_cameras_simulator_node_tumtraf({{"s110_n_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "tumtraf_v2x_cooperative_perception_dataset" / "train" / "images" / "s110_camera_basler_north_8mm"},
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
		ImageTrackerNode track;
		TrackToTrackFusionNode fusion({{"s110_n_cam_8", {config::projection_matrix_s110_base_north_into_s110_n_cam_8, config::affine_transformation_utm_to_s110_base_north}},
		    {"s110_o_cam_8", {config::projection_matrix_s110_base_north_into_s110_o_cam_8, config::affine_transformation_utm_to_s110_base_north}},
		    {"s110_s_cam_8", {config::projection_matrix_s110_base_north_into_s110_s_cam_8, config::affine_transformation_utm_to_s110_base_north}},
		    {"s110_w_cam_8", {config::projection_matrix_s110_base_north_into_s110_w_cam_8, config::affine_transformation_utm_to_s110_base_north}}});

		BirdEyeVisualizationNode<CompactObjects> vis(map, utm_to_image);
		ImageVisualizationNode img([](ImageData const& data) { return data.source == "bird"; });

		cams.asynchronously_connect(down);
		down.asynchronously_connect(yolo);
		yolo.asynchronously_connect(track);
		track.synchronously_connect(fusion);
		fusion.asynchronously_connect(vis);
		vis.synchronously_connect(img);

		auto cams_thread = cams();
		auto down_thread = down();
		auto yolo_thread = yolo();
		auto track_thread = track();
		auto vis_thread = vis();

		for (auto timestamp = std::chrono::system_clock::now() + 40s; std::chrono::system_clock::now() < timestamp; std::this_thread::yield()) g_main_context_iteration(NULL, true);
	}

	{
		CamerasSimulatorNode cams =
		    make_cameras_simulator_node_tumtraf({{"s110_n_cam_8", std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "tumtraf_v2x_cooperative_perception_dataset" / "train" / "images" / "s110_camera_basler_north_8mm"},
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
		ImageTrackerNode track;
		ImageTrackerResultsVisualization trackvis;
		ImageVisualizationNode img([](ImageData const& data) { return data.source == "s110_s_cam_8"; });

		cams.asynchronously_connect(down);
		down.asynchronously_connect(yolo);
		yolo.asynchronously_connect(track);
		track.synchronously_connect(trackvis);
		cams.synchronously_connect(trackvis);
		trackvis.synchronously_connect(img);

		auto cams_thread = cams();
		auto down_thread = down();
		auto yolo_thread = yolo();
		auto track_thread = track();

		for (auto timestamp = std::chrono::system_clock::now() + 40s; std::chrono::system_clock::now() < timestamp; std::this_thread::yield()) g_main_context_iteration(NULL, true);
	}
}