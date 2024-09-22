#pragma once

#include <map>
#include <opencv2/core/mat.hpp>
#include <opencv2/gapi.hpp>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

#include "node.h"

class BayerBG8Preprocessing : public InputOutputNode<ImageDataRaw, ImageData> {
	struct HeightWidthConfig {
		int height;
		int width;
	};

	// name -> height, width
	std::map<std::string, HeightWidthConfig> const config;

   public:
	explicit BayerBG8Preprocessing(std::remove_const<decltype(config)>::type&& config) : config(std::move(config)) {}

	ImageData function(ImageDataRaw const& data) final {
		// const cast is allowed here because vector is not changed
		cv::Mat const bayer_image(config.at(data.source).height, config.at(data.source).width, CV_8UC1, const_cast<std::uint8_t*>(data.image_raw.data()));

		ImageData ret;
		ret.timestamp = data.timestamp;
		ret.source = data.source;
		cv::demosaicing(bayer_image, ret.image, cv::COLOR_BayerBG2BGR);

		return ret;
	}
};
