#include <EigenUtils.h>

#include "BlockedArea.h"
#include "Config.h"
#include "DrawingUtils.h"
#include "ImageData.h"
#include "camera_simulator_node.h"
#include "transformation_node.h"
#include "undistort_node.h"
#include "visualization_node.h"
#include "yolo_node.h"

class SimpleAreaBlocking : public InputOutputNode<Detections2D, BlockedAreas> {
	Config const& config;

   public:
	explicit SimpleAreaBlocking(Config const& config) : config(config) {}

   private:
	BlockedAreas function(Detections2D const& data) final {
		BlockedAreas blocked_area_list{data.timestamp, {}};

		for (auto const& e : data.objects) {
			Eigen::Vector4d const position_left_top = config.affine_transformation_map_origin_to_utm() * config.affine_transformation_bases_to_map_origin(config.camera_config(data.source).base_name()) *
			                                          config.map_image_to_world_coordinate<double>(data.source, e.bbox.left, e.bbox.top, 0.);

			Eigen::Vector4d const position_right_top = config.affine_transformation_map_origin_to_utm() * config.affine_transformation_bases_to_map_origin(config.camera_config(data.source).base_name()) *
			                                           config.map_image_to_world_coordinate<double>(data.source, e.bbox.right, e.bbox.top, 0.);

			Eigen::Vector4d const position_right_bottom = config.affine_transformation_map_origin_to_utm() * config.affine_transformation_bases_to_map_origin(config.camera_config(data.source).base_name()) *
			                                              config.map_image_to_world_coordinate<double>(data.source, e.bbox.right, e.bbox.bottom, 0.);

			Eigen::Vector4d const position_left_bottom = config.affine_transformation_map_origin_to_utm() * config.affine_transformation_bases_to_map_origin(config.camera_config(data.source).base_name()) *
			                                             config.map_image_to_world_coordinate<double>(data.source, e.bbox.left, e.bbox.bottom, 0.);

			blocked_area_list.areas.push_back(BlockedArea{{{position_left_top(0), position_left_top(1)}, {position_right_top(0), position_right_top(1)}, {position_right_bottom(0), position_right_bottom(1)},
			    {position_left_bottom(0), position_left_bottom(1)}, {position_left_top(0), position_left_top(1)}}});
		}

		return blocked_area_list;
	}
};

class BlockedAreaVisualization : public OutputNode<BlockedAreas> {
	Config const& config;
	cv::Mat map;
	Eigen::Matrix<double, 4, 4> utm_to_image;
	boost::circular_buffer<std::vector<std::vector<cv::Point>>> drawing_fifo;

   public:
	explicit BlockedAreaVisualization(Config const& config, std::filesystem::path const& map_path = std::filesystem::path(CMAKE_SOURCE_DIR) / std::filesystem::path("data/visualization/2021-07-07_1490_Providentia_Plus_Plus_1_6.xodr"))
	    : config(config), drawing_fifo(5) {
		std::tie(map, utm_to_image) = draw_map(map_path, config);
		cv::imshow("display", map);
		cv::waitKey(100);
	}

   private:
	void output_function(BlockedAreas const& data) final {
		{
			std::vector<std::vector<cv::Point>> polys;

			for (auto const& blocked_area : data.areas) {
				std::vector<cv::Point> poly_points;
				for (auto const& e : blocked_area.area) {
					Eigen::Vector4d const position = utm_to_image * make_matrix<4, 1>(e.get<0>(), e.get<1>(), 0., 1.);
					poly_points.emplace_back(static_cast<int>(position(0)), static_cast<int>(position(1)));
				}
				polys.push_back(poly_points);
			}

			drawing_fifo.push_back(polys);
		}

		auto tmp = map.clone();
		for (auto const& polys : drawing_fifo) {
			for (auto const& poly_points : polys) {
				cv::polylines(tmp, poly_points, true, cv::Scalar(0, 0, 255));
			}
		}

		cv::imshow("display", tmp);
		cv::waitKey(10);
	}
};

int main() {
	Config config = make_config();

	CameraSimulator cam_n("s110_n_cam_8");
	CameraSimulator cam_o("s110_o_cam_8");
	CameraSimulator cam_s("s110_s_cam_8");
	CameraSimulator cam_w("s110_w_cam_8");
	Yolo yolo;
	UndistortDetections undistort(config);
	SimpleAreaBlocking area_blocking(config);
	BlockedAreaVisualization vis(config);

	cam_n += yolo;
	cam_o += yolo;
	cam_s += yolo;
	cam_w += yolo;

	yolo += undistort;
	undistort += area_blocking;

	area_blocking += vis;

	std::thread cam_n_thread(&CameraSimulator::operator(), &cam_n);
	std::thread cam_o_thread(&CameraSimulator::operator(), &cam_o);
	std::thread cam_s_thread(&CameraSimulator::operator(), &cam_s);
	std::thread cam_w_thread(&CameraSimulator::operator(), &cam_w);
	std::thread yolo_thread(&Yolo::operator(), &yolo);
	std::thread undistort_thread(&UndistortDetections::operator(), &undistort);
	std::thread area_blocking_thread(&SimpleAreaBlocking::operator(), &area_blocking);

	vis();
}
