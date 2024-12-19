#pragma once

#include <opencv2/opencv.hpp>

#include "ImageData.h"
#include "Processor.h"

class ImageUndistortionNode : public Processor<ImageData, ImageData> {
   public:
	struct UndistortionConfig {
		cv::Mat camera_matrix;
		cv::Mat distortion_values;
		cv::Mat new_camera_matrix;
		cv::Mat undistortion_map1;
		cv::Mat undistortion_map2;
	};

   private:
	std::map<std::string, UndistortionConfig> camera_matrix_distortion_values_new_camera_matrix_undistortion_maps_config;

   public:
	explicit ImageUndistortionNode(std::map<std::string, UndistortionConfig>&& camera_matrix_distortion_values_new_camera_matrix_undistortion_maps);

   private:
	ImageData process(ImageData const& data) final;
};