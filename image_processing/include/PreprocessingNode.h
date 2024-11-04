#pragma once

#include <map>
#include <opencv2/core/mat.hpp>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

#include "ImageData.h"
#include "ImageDataRaw.h"
#include "node.h"

class PreprocessingNode : public InputOutputNode<ImageDataRaw, ImageData> {
   public:
	struct HeightWidthConfig {
		int height;
		int width;
	};

	struct HeightWidthConversionConfig {
		int height;
		int width;
		cv::ColorConversionCodes color_conversion_code;
	};

   private:
	std::map<std::string, HeightWidthConversionConfig> const config;

   public:

	explicit PreprocessingNode(std::remove_const<decltype(config)>::type&& config) : config(std::move(config)) {}

   private:
	ImageData function(ImageDataRaw const& data) final;
};
