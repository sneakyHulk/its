#pragma once

#include <Eigen/Eigen>

#include "CompactObject.h"
#include "GlobalTrackerResult.h"
#include "ImageData.h"
#include "Processor.h"

template <typename Input>
class BirdEyeVisualizationNode : public Processor<Input, ImageData> {
	cv::Mat map;
	Eigen::Matrix<double, 4, 4> utm_to_image;

   public:
	explicit BirdEyeVisualizationNode(cv::Mat&& map, Eigen::Matrix<double, 4, 4>&& utm_to_image) : map(std::forward<decltype(map)>(map)), utm_to_image(std::forward<decltype(utm_to_image)>(utm_to_image)) {}

   private:
	ImageData process(Input const& data) final;
};