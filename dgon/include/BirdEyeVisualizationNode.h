#pragma once

#include "CompactObject.h"
#include "ImageData.h"
#include "KalmanBoxSourceTrack.h"
#include "node.h"
#include "opencv2/opencv.hpp"

class BirdEyeVisualizationNode : public Runner<CompactObjects> {
	cv::Mat _map;
	Eigen::Matrix<double, 4, 4> _utm_to_image;

   public:
	explicit BirdEyeVisualizationNode(cv::Mat map, Eigen::Matrix<double, 4, 4> utm_to_image);

   private:
	void run(CompactObjects const& data) final;
};