#pragma once

#include <opencv2/opencv.hpp>

#include "ImageData.h"
#include "node.h"

class VideoVisualization : public OutputNode<ImageData> {
	static const int max_frames = 100;
	int current_frame = max_frames;
	cv::VideoWriter video;

   public:
	VideoVisualization() = default;

   private:
	void output_function(ImageData const& data) final;
};