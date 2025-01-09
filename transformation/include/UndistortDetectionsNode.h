#pragma once

#include <opencv2/opencv.hpp>
#include <ranges>
#include <tuple>
#include <vector>

#include "Detection2D.h"
#include "Processor.h"

/**
 * @class UndistortDetectionsNode
 * @brief This class removes distortion from detected bounding box points.
 *
 * Rather than removing distortion of the entire image i.e. all image points, it only does it to the bounding box of the detected objects.
 * This has a similar outcome i.e. the accuracy is comparable.
 */
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
	explicit UndistortDetectionsNode(std::map<std::string, UndistortionConfig>&& camera_matrix_distortion_values_new_camera_matrix);

   private:
	Detections2D process(Detections2D const& data) final;
};