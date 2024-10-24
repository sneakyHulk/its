#include "ImageUndistortionNode.h"

ImageUndistortionNode::ImageUndistortionNode(std::map<std::string, UndistortionConfig>&& config) {
	for (auto const& [cam_name, undistortion_config] : config) {
		cv::Mat camera_matrix = cv::Mat_<double>(3, 3);
		for (auto i = 0; i < camera_matrix.rows; ++i) {
			for (auto j = 0; j < camera_matrix.cols; ++j) {
				camera_matrix.at<double>(i, j) = undistortion_config.camera_matrix[i * 3 + j];
			}
		}
		_undistortion_config[cam_name].camera_matrix = camera_matrix;

		cv::Mat distortion_values;
		for (auto i = 0; i < undistortion_config.distortion_values.size(); ++i) {
			distortion_values.push_back(undistortion_config.distortion_values[i]);
		}
		_undistortion_config[cam_name].distortion_values = distortion_values;

		cv::Size image_size(undistortion_config.width, undistortion_config.height);
		cv::Mat new_camera_matrix = cv::getOptimalNewCameraMatrix(_undistortion_config[cam_name].camera_matrix, _undistortion_config[cam_name].distortion_values, image_size, 0., image_size);
		_undistortion_config[cam_name].new_camera_matrix = new_camera_matrix;

		cv::initUndistortRectifyMap(_undistortion_config[cam_name].camera_matrix, _undistortion_config[cam_name].distortion_values, cv::Mat_<double>::eye(3, 3), _undistortion_config[cam_name].new_camera_matrix, image_size, CV_32F,
		    _undistortion_config[cam_name].undistortion_map1, _undistortion_config[cam_name].undistortion_map2);
	}
}

ImageData ImageUndistortionNode::function(ImageData const& data) {
	ImageData ret;
	ret.timestamp = data.timestamp;
	ret.source = data.source;
	cv::remap(data.image, ret.image, _undistortion_config[data.source].undistortion_map1, _undistortion_config[data.source].undistortion_map2, cv::INTER_LINEAR);

	return ret;
}