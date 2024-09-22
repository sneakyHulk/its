#pragma once

#include "ImageData.h"
#include "ImageTrackerNode.h"
#include "node.h"

class TrackingVisualizationNode : public OutputNodePair<ImageData, ImageTrackerResults2> {
	std::function<bool(ImageData const&)> _image_mask;

   public:
	TrackingVisualizationNode(std::function<bool(ImageData const&)> image_mask = [](ImageData const&) { return true; }) : _image_mask(image_mask) {}
	bool output_function(ImageData const& data, ImageTrackerResults2 const& results) final {
		if (!_image_mask(data)) return false;
		if (data.source != results.source) return false;
		if (data.timestamp != results.timestamp) return false;

		auto img = data.image.clone();

		cv::putText(img, "Image is still distorted, but Tracking is undistorted, thus edges look very wrong", cv::Point2d(0, 0), cv::FONT_HERSHEY_DUPLEX, 1.0, cv::Scalar_<int>(0, 0, 0), 1);

		for (auto const& object : results.objects) {
			cv::rectangle(img, cv::Point2d(object.state().left, object.state().top), cv::Point2d(object.state().right, object.state().bottom), cv::Scalar_<int>(0, 0, 255), 1);
			cv::rectangle(img, cv::Point2d(object.detection().bbox.left, object.detection().bbox.top), cv::Point2d(object.detection().bbox.right, object.detection().bbox.bottom), cv::Scalar_<int>(0, 0, 255), 5);

			cv::putText(img, std::to_string(object.id()), cv::Point2d(object.state().left, object.state().top), cv::FONT_HERSHEY_DUPLEX, 1.0, cv::Scalar_<int>(0, 0, 0), 1);
		}

		cv::imshow("TrackingVisualizationNode", img);
		cv::waitKey(10);

		return true;
	}
};