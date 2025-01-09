#include "UndistortDetectionsNode.h"

UndistortDetectionsNode::UndistortDetectionsNode(std::map<std::string, UndistortionConfig>&& camera_matrix_distortion_values_new_camera_matrix)
    : camera_matrix_distortion_values_new_camera_matrix_config(std::forward<decltype(camera_matrix_distortion_values_new_camera_matrix)>(camera_matrix_distortion_values_new_camera_matrix)) {}

/**
 * @brief Removes the distortion of the bounding box points of the detected objects.
 *
 * @param data The bounding boxes of the objects that have been detected in the distorted image.
 * @return The bounding boxes with removed distortion.
 */
Detections2D UndistortDetectionsNode::process(const Detections2D& data) {
	auto ret = data;

	for (auto& object : ret.objects) {
		std::vector<cv::Point2d> out(2);
		std::vector<cv::Point2d> distorted_points = {{object.bbox.left, object.bbox.top}, {object.bbox.right, object.bbox.bottom}};

		cv::undistortPoints(distorted_points, out, camera_matrix_distortion_values_new_camera_matrix_config.at(data.source).camera_matrix, camera_matrix_distortion_values_new_camera_matrix_config.at(data.source).distortion_values,
		    cv::Mat_<double>::eye(3, 3), camera_matrix_distortion_values_new_camera_matrix_config.at(data.source).new_camera_matrix);

		object.bbox.left = out[0].x;
		object.bbox.top = out[0].y;
		object.bbox.right = out[1].x;
		object.bbox.bottom = out[1].y;
	}

	return ret;
}
