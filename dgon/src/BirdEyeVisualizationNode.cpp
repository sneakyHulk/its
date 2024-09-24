#include "BirdEyeVisualizationNode.h"

#include "EigenUtils.h"
#include "common_literals.h"

// BirdEyeVisualizationNode::BirdEyeVisualizationNode(const Config &config, const std::filesystem::path &map_path) : config(config), drawing_fifo(5) { std::tie(map, utm_to_image) = draw_map(map_path, config); }
BirdEyeVisualizationNode::BirdEyeVisualizationNode(cv::Mat map, Eigen::Matrix<double, 4, 4> utm_to_image) : _map(map), _utm_to_image(utm_to_image) {}

void BirdEyeVisualizationNode::output_function(CompactObjects const &data) {
	auto tmp = _map.clone();
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

		Eigen::Vector4d const position = _utm_to_image * make_matrix<4, 1>(object.position[0], object.position[1], 0., 1.);  // * config.affine_transformation_map_origin_to_utm()

		cv::circle(tmp, cv::Point(static_cast<int>(position(0)), static_cast<int>(position(1))), 1, color, 10);
	}

	cv::imshow("bird", tmp);
	cv::waitKey(10);
}