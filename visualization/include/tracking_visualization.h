#pragma once

#include <chrono>
#include <cstring>
#include <opencv2/opencv.hpp>
#include <utility>
#include <vector>

#include "common.h"
#include "common_output.h"
#include "ImageData.h"
#include "ImageTrackerResult.h"
#include "node.h"

class ImageTrackingVisualizationHelper : public OutputPtrNode<ImageData> {
	friend class ImageTrackingVisualization;

	std::vector<std::shared_ptr<ImageData const>> image_buffer;

	virtual void output_function(std::shared_ptr<ImageData const> const& data) final { image_buffer.push_back(data); }
};

class ImageTrackingVisualization : public OutputNode<ImageTrackerResults> {
	ImageTrackingVisualizationHelper const& helper;
	std::string const cam_name;

   public:
	explicit ImageTrackingVisualization(ImageTrackingVisualizationHelper const& helper, std::string cam_name) : helper(helper), cam_name(std::move(cam_name)) {}

   private:
	void output_function(ImageTrackerResults const& data) final {
		if (cam_name == data.source) {
			for (auto const& e : helper.image_buffer) {
				if (e->timestamp == data.timestamp) {
					auto img = e->image.clone();

					for (auto const& res : data.objects) {
						res.matched ? cv::rectangle(img, cv::Point2d(res.bbox.left, res.bbox.top), cv::Point2d(res.bbox.right, res.bbox.bottom), cv::Scalar_<int>(0, 0, 255), 5)
						            : cv::rectangle(img, cv::Point2d(res.bbox.left, res.bbox.top), cv::Point2d(res.bbox.right, res.bbox.bottom), cv::Scalar_<int>(0, 0, 255), 1);

						cv::putText(img, std::to_string(res.id), cv::Point2d(res.bbox.left, res.bbox.top), cv::FONT_HERSHEY_DUPLEX, 1.0, cv::Scalar_<int>(0, 0, 0), 1);
					}

					cv::imshow("ImageTrackingVisualization", img);
					cv::waitKey(10);
					return;
				}
			}
		}
	}
};