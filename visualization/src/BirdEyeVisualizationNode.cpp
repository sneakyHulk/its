#include "BirdEyeVisualizationNode.h"

#include "EigenUtils.h"
#include "common_literals.h"

using namespace std::chrono_literals;

/**
 * @brief Displays the different road user in the bird's eye view image.
 *
 * @param data The positions of the road users.
 * @return The bird's eye view image with the displayed road user.
 */
template <>
ImageData BirdEyeVisualizationNode<CompactObjects>::process(CompactObjects const &data) {
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

		Eigen::Vector4d const position = utm_to_image * make_matrix<4, 1>(object.position[0], object.position[1], object.position[2], 1.);

		cv::circle(tmp, cv::Point(static_cast<int>(position[0]), static_cast<int>(position[1])), 1, color, 10);
	}

	return ImageData{tmp, data.timestamp, "bird"};
}

/**
 * @brief Displays the different road user in the bird's eye view image.
 *
 * @param data The positions of the road users.
 * @return The bird's eye view image with the displayed road user.
 */
template <>
ImageData BirdEyeVisualizationNode<GlobalTrackerResults>::process(GlobalTrackerResults const &data) {
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

		Eigen::Vector4d const position = utm_to_image * make_matrix<4, 1>(object.position[0], object.position[1], 0., 1.);

		cv::circle(tmp, cv::Point(static_cast<int>(position(0)), static_cast<int>(position(1))), 1, color, 10);
	}

	return ImageData{tmp, data.timestamp, "bird"};
}

/**
 * @brief Defines the BirdEyeVisualizationNode class for GlobalTrackerResults structure.
 */
template class BirdEyeVisualizationNode<GlobalTrackerResults>;
/**
 * @brief Defines the BirdEyeVisualizationNode class for CompactObjects structure.
 */
template class BirdEyeVisualizationNode<CompactObjects>;