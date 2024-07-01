#pragma once

#include <utility>
#include <vector>

#include "ImageData.h"
#include "ImageTrackerResult.h"
#include "node.h"

class ImageTrackingVisualizationHelper : public OutputPtrNode<ImageData> {
	friend class ImageTrackingVisualization;

	std::vector<std::shared_ptr<ImageData const>> image_buffer;

	void output_function(std::shared_ptr<ImageData const> const& data) final;
};

class ImageTrackingVisualization : public OutputNode<ImageTrackerResults> {
	ImageTrackingVisualizationHelper const& helper;
	std::string const cam_name;

   public:
	explicit ImageTrackingVisualization(ImageTrackingVisualizationHelper const& helper, std::string cam_name);

   private:
	void output_function(ImageTrackerResults const& data) final;
};