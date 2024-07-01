#include "visualization/visualization.h"

#include <chrono>
#include <ranges>

#include "common_literals.h"
#include "common_output.h"
#include "transformation/DrawingUtils.h"
#include "transformation/EigenUtils.h"
using namespace std::chrono_literals;

void ImageVisualization::output_function(ImageData const& data) {
	common::println(
	    "Time taken = ", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::nanoseconds(std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now()).time_since_epoch().count() - data.timestamp)));
	cv::imshow("Display window", data.image);
	cv::waitKey(1);
}
Visualization2D::Visualization2D(Config const& config, std::filesystem::path const& map_path) : config(config), drawing_fifo(5) {
	std::tie(map, utm_to_image) = draw_map(std::filesystem::path(CMAKE_SOURCE_DIR) / std::filesystem::path("data/visualization/2021-07-07_1490_Providentia_Plus_Plus_1_6.xodr"), config);
	cv::imshow("display", map);
	cv::waitKey(100);
}
void Visualization2D::output_function(CompactObjects const& data) {
	CompactObjects display_objects = data;
	for (auto& e : display_objects.objects) {
		Eigen::Vector4d const position = utm_to_image * make_matrix<4, 1>(e.position[0], e.position[1], e.position[2], 1.);

		e.position[0] = position(0);
		e.position[1] = position(1);
	}

	drawing_fifo.push_back(display_objects);

	auto tmp = map.clone();
	for (auto const& objects : drawing_fifo) {
		for (auto const& object : objects.objects) {
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
