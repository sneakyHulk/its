#pragma once

#include <opencv2/opencv.hpp>

#include "ImageData.h"
#include "node.h"

class ImageUndistortionNode : public Processor<ImageData, ImageData> {
   public:
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

	explicit ImageUndistortionNode(std::map<std::string, UndistortionConfig>&& config);

   private:
	ImageData process(ImageData const& data) final;

	std::map<std::string, UndistortionConfigInternal> _undistortion_config;
};