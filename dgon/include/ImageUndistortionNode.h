#pragma once

#include <opencv2/opencv.hpp>

#include "AfterReturnTimeMeasure.h"
#include "ImageData.h"
#include "node.h"

class ImageUndistortionNode : public InputOutputNode<ImageData, ImageData> {
	struct UndistortionConfig {
		std::vector<double> camera_matrix;
		std::vector<double> distortion_values;
		int height;
		int width;
	};

	struct UndistortionConfigInternal {
		cv::Mat camera_matrix;
		cv::Mat distortion_values;
		cv::Mat new_camera_matrix;
		cv::Mat undistortion_map1;
		cv::Mat undistortion_map2;
	};

	std::map<std::string, UndistortionConfigInternal> _undistortion_config;

   public:
	ImageUndistortionNode(std::map<std::string, UndistortionConfig>&& config) {
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

	ImageData function(ImageData const& data) {
		AfterReturnTimeMeasure after(data.timestamp);

		ImageData ret;
		ret.timestamp = data.timestamp;
		ret.source = data.source;
		cv::remap(data.image, ret.image, _undistortion_config[data.source].undistortion_map1, _undistortion_config[data.source].undistortion_map2, cv::INTER_LINEAR);

		return ret;
	}
};