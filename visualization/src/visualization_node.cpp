#include "visualization_node.h"

#include <chrono>
#include <ranges>

#include "DrawingUtils.h"
#include "EigenUtils.h"
#include "common_literals.h"
#include "common_output.h"

using namespace std::chrono_literals;

Visualization2D::Visualization2D(Config const &config, std::filesystem::path &&map_path) : config(config), drawing_fifo(5) {
	std::tie(map, utm_to_image) = draw_map(std::move(map_path), config);
	cv::imshow("display", map);
	cv::waitKey(100);
}

void Visualization2D::run(CompactObjects const &data) {
	CompactObjects display_objects = data;
	for (auto &e : display_objects.objects) {
		Eigen::Vector4d const position = utm_to_image * make_matrix<4, 1>(e.position[0], e.position[1], e.position[2], 1.);

		e.position[0] = position(0);
		e.position[1] = position(1);
	}

	drawing_fifo.push_back(display_objects);

	auto tmp = map.clone();
	for (auto const &objects : drawing_fifo) {
		for (auto const &object : objects.objects) {
			cv::Scalar color;
			switch (object.object_class) {
				case 0_u8: color = cv::Scalar(0, 0, 255); break;
				case 1_u8: color = cv::Scalar(0, 0, 150); break;
				case 2_u8: color = cv::Scalar(255, 0, 0); break;
				case 3_u8: color = cv::Scalar(150, 0, 0); break;
				case 5_u8: color = cv::Scalar(0, 150, 0); break;
				case 7_u8: color = cv::Scalar(0, 255, 0); break;
				default: throw;
			}
			cv::circle(tmp, cv::Point(object.position[0], object.position[1]), 1, color, 10);
		}
	}

	cv::imshow("display", tmp);
	cv::waitKey(10);
}

BirdEyeVisualization::BirdEyeVisualization(const Config &config, std::filesystem::path &&map_path) : config(config), drawing_fifo(5) { std::tie(map, utm_to_image) = draw_map(std::move(map_path), config); }

ImageData BirdEyeVisualization::process(const GlobalTrackerResults &data) {
	auto tmp = map.clone();
	for (auto const &object : data.objects) {
		cv::Scalar color;
		switch (object.object_class) {
			case 0_u8: color = cv::Scalar(0, 0, 255); break;
			case 1_u8: color = cv::Scalar(0, 0, 150); break;
			case 2_u8: color = cv::Scalar(255, 0, 0); break;
			case 3_u8: color = cv::Scalar(150, 0, 0); break;
			case 5_u8: color = cv::Scalar(0, 150, 0); break;
			case 7_u8: color = cv::Scalar(0, 255, 0); break;
			default: throw;
		}

		Eigen::Vector4d const position = utm_to_image * config.affine_transformation_map_origin_to_utm() * make_matrix<4, 1>(object.position[0], object.position[1], 0., 1.);

		cv::circle(tmp, cv::Point(static_cast<int>(position(0)), static_cast<int>(position(1))), 1, color, 10);
	}

	return ImageData{tmp, data.timestamp, "bird"};
}
