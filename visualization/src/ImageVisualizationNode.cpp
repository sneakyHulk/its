#include "ImageVisualizationNode.h"

#include <filesystem>
#include <utility>

void ImageVisualizationNode::output_function(ImageData const &data) {
	// common::println(
	//     "Time taken = ", std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::nanoseconds(std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now()).time_since_epoch().count() -
	//     data.timestamp)));

	if (!_image_mask(data)) return;
	cv::imshow(display_name, data.image);
	cv::waitKey(1);
}
ImageVisualizationNode::ImageVisualizationNode(std::function<bool(const ImageData &)> image_mask, std::source_location const location) : _image_mask(std::move(image_mask)) {
	display_name = common::stringprint("ImageVisualization of ", std::filesystem::path(location.file_name()).stem().string(), '(', location.line(), ':', location.column(), ")");
}
