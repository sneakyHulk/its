#pragma once

#include <Eigen/Dense>
#include <map>
#include <opencv2/core/eigen.hpp>
#include <opencv2/opencv.hpp>

#include "Detection2D.h"
#include "node.h"

class UndistortionNode : public InputOutputNode<Detections2D, Detections2D> {
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
	};

	std::map<std::string, UndistortionConfigInternal> _undistortion_config;

	[[nodiscard]] std::tuple<double, double> undistort_point(std::string const& source, double const x, double const y) const {
		std::vector<cv::Point2d> out(1);
		std::vector<cv::Point2d> distorted_point = {{x, y}};

		cv::undistortPoints(distorted_point, out, _undistortion_config.at(source).camera_matrix, _undistortion_config.at(source).distortion_values, cv::Mat_<double>::eye(3, 3), _undistortion_config.at(source).new_camera_matrix);

		return {out.front().x, out.front().y};
	}

   public:
	explicit UndistortionNode(std::map<std::string, UndistortionConfig>&& config) {
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
		}
	}

	Detections2D function(Detections2D const& data) final {
		Detections2D ret = data;

		for (auto& detection : ret.objects) {
			auto& [left, top, right, bottom] = detection.bbox;

			std::tie(left, top) = undistort_point(data.source, left, top);
			std::tie(right, bottom) = undistort_point(data.source, right, bottom);
		}

		return ret;
	}
};