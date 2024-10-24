#pragma once

#include <functional>
#include <source_location>

#include "ImageData.h"
#include "node.h"

class ImageVisualizationNode : public OutputNode<ImageData> {
	std::function<bool(ImageData const&)> _image_mask;
	std::string display_name;

   public:
	explicit ImageVisualizationNode(std::function<bool(ImageData const&)> image_mask = [](ImageData const&) { return true; }, std::source_location const location = std::source_location::current());

	void output_function(ImageData const& data) final;
};
