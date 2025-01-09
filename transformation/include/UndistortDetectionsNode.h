#pragma once

#include <opencv2/opencv.hpp>
#include <ranges>
#include <tuple>
#include <vector>

#include "Detection2D.h"
#include "Processor.h"

class UndistortDetectionsNode : public Processor<Detections2D, Detections2D> {
   public:
	struct UndistortionConfig {
		cv::Mat camera_matrix;
		cv::Mat distortion_values;
		cv::Mat new_camera_matrix;
	};

   private:
	std::map<std::string, UndistortionConfig> camera_matrix_distortion_values_new_camera_matrix_config;

   public:
	explicit UndistortDetectionsNode(std::map<std::string, UndistortionConfig>&& camera_matrix_distortion_values_new_camera_matrix)
	    : camera_matrix_distortion_values_new_camera_matrix_config(std::forward<decltype(camera_matrix_distortion_values_new_camera_matrix)>(camera_matrix_distortion_values_new_camera_matrix)) {}

   private:
	Detections2D process(Detections2D const& data) final {
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
};