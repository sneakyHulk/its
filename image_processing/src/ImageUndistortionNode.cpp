#include "ImageUndistortionNode.h"

ImageUndistortionNode::ImageUndistortionNode(std::map<std::string, UndistortionConfig>&& camera_matrix_distortion_values_new_camera_matrix_undistortion_maps)
    : camera_matrix_distortion_values_new_camera_matrix_undistortion_maps_config(
          std::forward<decltype(camera_matrix_distortion_values_new_camera_matrix_undistortion_maps_config)>(camera_matrix_distortion_values_new_camera_matrix_undistortion_maps)) {}

/**
 * @brief Performs undistortion of the input image. See https://docs.opencv.org/4.x/d9/d0c/group__calib3d.html.
 *
 * @param data The distorted image data.
 * @return The undistorted image data.
 */
ImageData ImageUndistortionNode::process(ImageData const& data) {
	cv::Mat distorted_image = data.image.clone();

	ImageData ret;
	ret.timestamp = data.timestamp;
	ret.source = data.source;
	cv::remap(distorted_image, ret.image, camera_matrix_distortion_values_new_camera_matrix_undistortion_maps_config.at(data.source).undistortion_map1,
	    camera_matrix_distortion_values_new_camera_matrix_undistortion_maps_config.at(data.source).undistortion_map2, cv::INTER_LINEAR);

	return ret;
}