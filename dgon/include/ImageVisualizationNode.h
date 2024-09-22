#pragma once

#include <functional>

#include "ImageData.h"
#include "node.h"

class ImageVisualizationNode : public OutputNode<ImageData> {
	std::function<bool(ImageData const&)> _image_mask;

   public:
	explicit ImageVisualizationNode(std::function<bool(ImageData const&)> image_mask = [](ImageData const&) { return true; });

	void output_function(ImageData const& data) final;
};
