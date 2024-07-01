#pragma once

#include "CompactObject.h"
#include "Detection2D.h"
#include "GlobalTrackerResult.h"
#include "ImageTrackerResult.h"
#include "node.h"
#include "Config.h"

class DetectionTransformation final : public InputOutputNode<Detections2D, CompactObjects> {
	Config const& config;

   public:
	explicit DetectionTransformation(Config const& config);

   private:
	CompactObjects function(Detections2D const& data) final;
};

class ImageTrackingTransformation final : public InputOutputNode<ImageTrackerResults, CompactObjects> {
	Config const& config;

   public:
	explicit ImageTrackingTransformation(Config const& config);

   private:
	CompactObjects function(ImageTrackerResults const& data) final;
};

class GlobalTrackingTransformation final : public InputOutputNode<GlobalTrackerResults, CompactObjects> {
	Config const& config;

   public:
	explicit GlobalTrackingTransformation(Config const& config);

   private:
	CompactObjects function(GlobalTrackerResults const& data) final;
};