#pragma once

#include "node.h"
#include "ImageTrackerNode.h"
#include "BirdEyeVisualizationData.h"

class TrackToTrackFusionNode :public InputOutputNode<ImageTrackerResults2, BirdEyeVisualizationDataPoints> {
	struct TransformationConfig {

	};

	TrackToTrackFusionNode(std::map<std::string, TransformationConfig> config) {

	}

	BirdEyeVisualizationDataPoints function(ImageTrackerResults2 const& data) final {
		BirdEyeVisualizationDataPoints ret;





		ret.objects.emplace_back({x, y}, {vx, vy}, id, object_class);


	}

};