#include "ImagePreprocessingNode.h"

ImageData ImagePreprocessingNode::process(const ImageDataRaw& data) {
	// const cast is allowed here because vector is not changed
	cv::Mat const bayer_image(height_width_conversion_config.at(data.source).height, height_width_conversion_config.at(data.source).width, CV_8UC1, const_cast<std::uint8_t*>(data.image_raw.data()));

	ImageData ret;
	ret.timestamp = data.timestamp;
	ret.source = data.source;
	cv::demosaicing(bayer_image, ret.image, height_width_conversion_config.at(data.source).color_conversion_code);

	return ret;
}
