#pragma once

#include <opencv2/opencv.hpp>

#include "ImageData.h"
#include "Runner.h"

/**
 * @class VideoVisualizationNode
 * @brief A node for creating a video from incoming image data.
 */
class VideoVisualizationNode : public Runner<ImageData> {
	static const int max_frames = 1000;
	int current_frame = max_frames;
	cv::VideoWriter video;

   public:
	VideoVisualizationNode() = default;

   private:
	void run(ImageData const& data) final;
};