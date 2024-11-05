#pragma once

#include <opencv2/opencv.hpp>

#include "ImageData.h"
#include "node.h"

class VideoVisualization : public Runner<ImageData> {
	static const int max_frames = 1000;
	int current_frame = max_frames;
	cv::VideoWriter video;

   public:
	VideoVisualization() = default;

   private:
	void run(ImageData const& data) final;
};