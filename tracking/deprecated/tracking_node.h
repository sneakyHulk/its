#pragma once

#include <map>
#include <string>
#include <vector>

#include "CompactObject.h"
#include "Config.h"
#include "Detection2D.h"
#include "GlobalTrackerResult.h"
#include "ImageTrackerResult.h"
//#include "Sort.h"
#include "association_functions.h"
#include "node.h"
#include "KalmanBoxTracker.h"

//class SortTracking final : public InputOutputNode<Detections2D, ImageTrackerResults> {
//	Config const& config;
//	std::map<std::string, Sort<>> trackers;
//
//   public:
//	explicit SortTracking(Config const& config);
//
//   private:
//	ImageTrackerResults function(Detections2D const& data) final;
//};

class GlobalImageTracking final : public InputOutputNode<Detections2D, GlobalTrackerResults> {
	Config const& config;
	std::map<std::string, std::vector<KalmanBoxTracker<3>>> image_trackers;
	std::uint64_t old_timestamp = 0;
	double association_threshold = 0.1;
	double (*association_function)(BoundingBoxXYXY const& bbox1, BoundingBoxXYXY const& bbox2) = iou;

   public:
	explicit GlobalImageTracking(Config const& config);

   private:
	GlobalTrackerResults function(Detections2D const& data) final;
};