#pragma once

#include <algorithm>

#include "AfterReturnTimeMeasure.h"
#include "ImageTrackerResult.h"
#include "KalmanBoxSourceTrack.h"
#include "association_functions.h"
#include "linear_assignment.h"
#include "node.h"

struct ImageTrackerResults2 {
	std::uint64_t timestamp;                    // UTC timestamp since epoch in ns
	std::string source;                         // sensor source of detections
	std::vector<KalmanBoxSourceTrack> objects;  // vector of tracks
};

template <std::uint64_t max_age = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::milliseconds(700)).count()>
class ImageTrackerNode : public InputOutputNode<Detections2D, ImageTrackerResults2> {
	std::map<std::string, std::vector<KalmanBoxSourceTrack>> multiple_cameras_tracks;

   public:
	ImageTrackerNode() = default;

	ImageTrackerResults2 function(Detections2D const& data) {
		AfterReturnTimeMeasure after(data.timestamp);

		// common::println("Time taken = ",
		//     std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::nanoseconds(std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now()).time_since_epoch().count() - data.timestamp)));

		auto& tracks = multiple_cameras_tracks[data.source];

		for (auto track = tracks.begin(); track != tracks.end(); ++track) {
			if (data.timestamp - track->last_update() > max_age)
				track = --tracks.erase(track);
			else if (!track->predict(data.timestamp))
				track = --tracks.erase(track);
		}

		Eigen::MatrixXd association_matrix = Eigen::MatrixXd::Ones(tracks.size(), data.objects.size());
		for (auto i = 0; i < tracks.size(); ++i) {
			for (auto j = 0; j < data.objects.size(); ++j) {
				auto state = tracks[i].state();
				auto value = iou(state, data.objects[j].bbox);
				if (std::isnan(value)) {
					common::println("shit");
					throw false;
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

		ImageTrackerResults2 ret;
		ret.source = data.source;
		ret.timestamp = data.timestamp;
		ret.objects = tracks;

		return ret;
	}
};