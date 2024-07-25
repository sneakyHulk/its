#pragma once

#include <utility>
#include <vector>

#include "ImageData.h"
#include "ImageTrackerResult.h"
#include "common_output.h"
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

class ImageTrackingVisualization2 : public OutputNodePair<ImageData, ImageTrackerResults> {
	bool output_function(ImageData const& data, ImageTrackerResults const& results) final {
		if (data.source != results.source) return false;
		if (data.timestamp != results.timestamp) return false;

		auto img = data.image.clone();

		cv::putText(img, "Image is still distorted, but Tracking is undistorted, thus edges look very wrong", cv::Point2d(0, 0), cv::FONT_HERSHEY_DUPLEX, 1.0, cv::Scalar_<int>(0, 0, 0), 1);


		for (auto const& res : results.objects) {
			res.matched ? cv::rectangle(img, cv::Point2d(res.bbox.left, res.bbox.top), cv::Point2d(res.bbox.right, res.bbox.bottom), cv::Scalar_<int>(0, 0, 255), 5)
			            : cv::rectangle(img, cv::Point2d(res.bbox.left, res.bbox.top), cv::Point2d(res.bbox.right, res.bbox.bottom), cv::Scalar_<int>(0, 0, 255), 1);

			cv::putText(img, std::to_string(res.id), cv::Point2d(res.bbox.left, res.bbox.top), cv::FONT_HERSHEY_DUPLEX, 1.0, cv::Scalar_<int>(0, 0, 0), 1);
		}

		cv::imshow("ImageTrackingVisualization", img);
		cv::waitKey(10);

		return true;
	}
};