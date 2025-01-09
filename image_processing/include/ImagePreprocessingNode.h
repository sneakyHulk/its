#pragma once

#include <map>
#include <opencv2/core/mat.hpp>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

#include "ImageData.h"
#include "ImageDataRaw.h"
#include "Processor.h"

/**
 * @class ImagePreprocessingNode
 * @brief This class converts raw image data.
 */
class ImagePreprocessingNode : public Processor<ImageDataRaw, ImageData> {
   public:
	struct HeightWidthConversionConfig {
		int height;
		int width;
		cv::ColorConversionCodes color_conversion_code;
	};

   private:
	std::map<std::string, HeightWidthConversionConfig> height_width_conversion_config;

   public:
	explicit ImagePreprocessingNode(std::map<std::string, HeightWidthConversionConfig>&& height_width_conversion);

   private:
	ImageData process(ImageDataRaw const& data) final;
};
