#pragma once

#include <utility>

#include "Detection2D.h"
#include "ImageData.h"
#include "node.h"
#include "KalmanBoxSourceTrack.h"
#include "association_functions.h"
#include "linear_assignment.h"

class ImageDetectionVisualizationNode : public OutputNodePair<ImageData, Detections2D> {
	std::function<bool(ImageData const&)> _image_mask;

   std::vector<KalmanBoxSourceTrack> tracks;

   public:
	explicit ImageDetectionVisualizationNode(std::function<bool(ImageData const&)> image_mask = [](ImageData const&) { return true; }) : _image_mask(std::move(image_mask)) {}
	bool output_function(ImageData const& data, Detections2D const& detections) final {
		if (!_image_mask(data)) return false;
		if (data.source != detections.source) return false;
		if (data.timestamp != detections.timestamp) return false;

		auto img = data.image.clone();

		cv::putText(img, "Image is still distorted, but Tracking is undistorted, thus edges look very wrong", cv::Point2d(0, 0), cv::FONT_HERSHEY_DUPLEX, 1.0, cv::Scalar_<int>(0, 0, 0), 1);

		for (auto const& object : detections.objects) {
			cv::rectangle(img, cv::Point2d(object.bbox.left, object.bbox.top), cv::Point2d(object.bbox.right, object.bbox.bottom), cv::Scalar_<int>(0, 0, 255), 1);
		}

		for (auto track = tracks.begin(); track != tracks.end(); ++track) {
			if (data.timestamp - track->last_update() > 700000000)
				track = --tracks.erase(track);
			else if (!track->predict(data.timestamp))
				track = --tracks.erase(track);
			//else
				//cv::rectangle(img, cv::Point2d(track->state().left, track->state().top), cv::Point2d(track->state().right, track->state().bottom), cv::Scalar_<int>(0, 255, 0), 1);
		}

		Eigen::MatrixXd association_matrix = Eigen::MatrixXd::Ones(tracks.size(), detections.objects.size());
		for (auto i = 0; i < tracks.size(); ++i) {
			for (auto j = 0; j < detections.objects.size(); ++j) {
				auto state = tracks[i].state();
				auto value = iou(state, detections.objects[j].bbox);

				association_matrix(i, j) -= value;
			}
		}

		common::println(association_matrix, "-------");

		auto const [matches, unmatched_tracks, unmatched_detections] = linear_assignment(association_matrix, 1.);

		// create new tracker from new not matched detections:
		for (auto const detection_index : unmatched_detections) {
			tracks.emplace_back(detections.objects[detection_index], data.timestamp);
			cv::rectangle(img, cv::Point2d(detections.objects[detection_index].bbox.left, detections.objects[detection_index].bbox.top), cv::Point2d(detections.objects[detection_index].bbox.right, detections.objects[detection_index].bbox.bottom), cv::Scalar_<int>(255, 0, 0), 2);

		}

		// update old tracker with matched detections:
		for (auto const& [tracker_index, detection_index] : matches) {
			tracks[tracker_index].update(detections.objects[detection_index]);
		}






		cv::imshow("ImageDetectionVisualizationNode", img);
		cv::waitKey(10);

		return true;
	}
};