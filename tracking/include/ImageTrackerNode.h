#pragma once

#include <algorithm>
#include <chrono>

#include "KalmanBoxSourceTrack.h"
#include "Processor.h"
#include "association_functions.h"
#include "linear_assignment.h"

using namespace std::chrono_literals;

struct ImageTrackerResults {
	std::uint64_t timestamp;                    // UTC timestamp since epoch in ns
	std::string source;                         // sensor source of detections
	std::vector<KalmanBoxSourceTrack> objects;  // vector of tracks
};

/**
 * @class ImageTrackerNode
 * @brief This class is responsible for tracking detected objects in images.
 * @tparam max_age The maximum age of tracks before they are removed.
 */
template <std::uint64_t max_age = std::chrono::duration_cast<std::chrono::nanoseconds>(700ms).count()>
class ImageTrackerNode : public Processor<Detections2D, ImageTrackerResults> {
	std::map<std::string, std::vector<KalmanBoxSourceTrack>> multiple_cameras_tracks;

   public:
	ImageTrackerNode() = default;

	/**
	 * @brief Does one iteration of the sort tracking algorithm.
	 */
	ImageTrackerResults process(Detections2D const& data) override {
		auto& tracks = multiple_cameras_tracks[data.source];

		// Deletes tracks which were updated > max age ago.
		for (auto track = tracks.begin(); track != tracks.end(); ++track) {
			if (data.timestamp - track->last_update() > max_age) track = --tracks.erase(track);
		}

		// Deletes tracks whose bounding box area is < 0
		for (auto track = tracks.begin(); track != tracks.end(); ++track) {
			try {
				track->predict(data.timestamp);
			} catch (common::Exception const& e) {
				track = --tracks.erase(track);
				common::println_warn_loc(e.what());
			}
		}

		// Produces the association matrix for every detected object.
		Eigen::MatrixXd association_matrix = Eigen::MatrixXd::Ones(tracks.size(), data.objects.size());
		for (auto i = 0; i < tracks.size(); ++i) {
			for (auto j = 0; j < data.objects.size(); ++j) {
				auto state = tracks[i].state();
				auto value = iou(state, data.objects[j].bbox);
				if (std::isnan(value)) {
					common::println_critical_loc("iou is nan!");
				}
				association_matrix(i, j) -= std::isnan(value) ? 0. : value;
			}
		}

		auto const [matches, unmatched_tracks, unmatched_detections] = linear_assignment(association_matrix, 1. - 0.05);

		// create new tracker from new not matched detections:
		for (auto const detection_index : unmatched_detections) {
			tracks.emplace_back(data.objects[detection_index], data.timestamp);
		}

		// update old tracker with matched detections:
		for (auto const& [tracker_index, detection_index] : matches) {
			tracks[tracker_index].update(data.objects[detection_index]);
		}

		ImageTrackerResults ret;
		ret.source = data.source;
		ret.timestamp = data.timestamp;
		ret.objects = tracks;

		return ret;
	}
};