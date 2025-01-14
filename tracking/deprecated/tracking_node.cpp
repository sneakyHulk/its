#include "tracking_node.h"

#include <Eigen/Eigen>

#include "linear_assignment.h"

// SortTracking::SortTracking(const Config& config) : config(config) {}
// ImageTrackerResults SortTracking::function(Detections2D const& data) {
//	// static thread_local std::map<std::string, Sort<>> trackers;  // will work for now because all Nodes are running on a different thread
//
//	if (!trackers.contains(data.source)) {
//		trackers.insert({data.source, Sort<>()});
//	}
//
//	ImageTrackerResults results;
//
//	results.objects = trackers.at(data.source).update(data.timestamp, data.objects);
//	results.source = data.source;
//	results.timestamp = data.timestamp;
//
//	return results;
// }

GlobalImageTracking::GlobalImageTracking(Config const& config) : config(config) {}
GlobalTrackerResults GlobalImageTracking::function(Detections2D const& data) {
	if (!image_trackers.contains(data.source)) {
		image_trackers.insert({data.source, {}});
	}

	// double const dt = std::chrono::duration<double>(std::chrono::nanoseconds(data.timestamp) - std::chrono::nanoseconds(old_timestamp)).count();
	// old_timestamp = data.timestamp;

	for (auto& [source, image_tracker] : image_trackers) {
		if (source == data.source) {
			for (auto track = image_tracker.begin(); track != image_tracker.end(); ++track)
				if (!track->predict(data.timestamp)) track = --image_tracker.erase(track);
		} else {
			for (auto track = image_tracker.begin(); track != image_tracker.end(); ++track)
				if (!track->predict_between(data.timestamp)) track = --image_tracker.erase(track);
		}
	}

	{
		auto& image_tracker = image_trackers[data.source];

		Eigen::MatrixXd association_matrix = Eigen::MatrixXd::Ones(image_tracker.size(), data.objects.size());
		for (auto i = 0; i < image_tracker.size(); ++i) {
			for (auto j = 0; j < data.objects.size(); ++j) {
				auto state = image_tracker[i].state();
				auto value = association_function(state, data.objects[j].bbox);
				if (std::isnan(value)) {
					auto x = 0;
				}
				association_matrix(i, j) -= std::isnan(value) ? 0. : value;
			}
		}
		auto const [matches, unmatched_tracks, unmatched_detections] = linear_assignment(association_matrix, 1. - association_threshold);

		// create new tracker from new not matched detections:
		for (auto const detection_index : unmatched_detections) {
			image_tracker.emplace_back(data.objects[detection_index].bbox, data.timestamp, data.objects[detection_index].object_class);
		}

		// update old tracker with matched detections:
		for (auto const& [tracker_index, detection_index] : matches) {
			image_tracker[tracker_index].update(data.objects[detection_index].bbox);
		}
	}

	GlobalTrackerResults res;
	res.timestamp = data.timestamp;

	for (auto& [source, image_tracker] : image_trackers)
		for (auto& track : image_tracker) {
			auto const [x, y] = track.position();
			Eigen::Vector4d const position = config.affine_transformation_bases_to_map_origin(config.camera_config(source).base_name()) * config.map_image_to_world_coordinate<double>(source, x, y, 0.);
			res.objects.emplace_back(track.id(), track.object_class(), track.matched(), std::array{position(0), position(1)}, track.velocity(), 0.);
		}

	return res;
}
