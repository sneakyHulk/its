#pragma once

#include <functional>

#include "ImageData.h"
#include "node.h"

class ImageVisualizationNode : public Runner<ImageData> {
	std::function<bool(ImageData const&)> _image_mask;

   public:
	explicit ImageVisualizationNode(std::function<bool(ImageData const&)> image_mask = [](ImageData const&) { return true; });

	void run(ImageData const& data) final;
};
