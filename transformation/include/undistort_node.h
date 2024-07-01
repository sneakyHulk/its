#pragma once

#include "Detection2D.h"
#include "node.h"
#include "Config.h"

class UndistortDetections : public InputOutputNode<Detections2D, Detections2D> {
	Config const& config;

   public:
	explicit UndistortDetections(Config const& config);

   private:
	Detections2D function(Detections2D const& data) final;
};