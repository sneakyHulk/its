#pragma once

#include <opencv2/opencv.hpp>

#include "ImageData.h"
#include "node.h"

template <int height, int width>
class DownscalingNode : public InputOutputNode<ImageData, ImageData> {
   public:
	DownscalingNode() = default;

	ImageData function(ImageData const& data) final {
		ImageData ret;
		ret.source = data.source;
		ret.timestamp = data.timestamp;

		float const ratio_h = static_cast<float>(height) / static_cast<float>(data.image.rows);
		float const ratio_w = static_cast<float>(width) / static_cast<float>(data.image.cols);
		float const resize_scale = std::min(ratio_h, ratio_w);

		int const new_shape_w = static_cast<int>(std::round(static_cast<float>(data.image.cols) * resize_scale));
		int const new_shape_h = static_cast<int>(std::round(static_cast<float>(data.image.rows) * resize_scale));
		float const padw = static_cast<float>(width - new_shape_w) / 2.f;
		float const padh = static_cast<float>(height - new_shape_h) / 2.f;

		int const top = static_cast<int>(std::round(padh - 0.1));
		int const bottom = static_cast<int>(std::round(padh + 0.1));
		int const left = static_cast<int>(std::round(padw - 0.1));
		int const right = static_cast<int>(std::round(padw + 0.1));

		cv::resize(data.image, ret.image, cv::Size(new_shape_w, new_shape_h), 0, 0, cv::INTER_AREA);
		cv::copyMakeBorder(ret.image, ret.image, top, bottom, left, right, cv::BORDER_CONSTANT, cv::Scalar(114.));

		return ret;
	}
};