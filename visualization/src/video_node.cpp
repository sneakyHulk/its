#include "video_node.h"
void VideoVisualization::output_function(ImageData const& data) {
	if (++current_frame > max_frames) {
		current_frame = 0;

		// will initialize and save video after frames > max_frames
		video =
		    cv::VideoWriter(std::string(CMAKE_SOURCE_DIR) + std::string("/result/video_") + data.source + "_" + std::to_string(data.timestamp) + ".mp4", cv::VideoWriter::fourcc('m', 'p', '4', 'v'), 15., cv::Size(data.width, data.height));
	}

	video.write(data.image);
}
