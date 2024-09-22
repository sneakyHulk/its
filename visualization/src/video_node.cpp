#include "video_node.h"

#include <filesystem>

void VideoVisualization::output_function(ImageData const& data) {
	if (++current_frame > max_frames) {
		current_frame = 0;

		// will initialize and save video after frames > max_frames
		video = cv::VideoWriter(std::filesystem::path(CMAKE_SOURCE_DIR) / std::filesystem::path(std::string("result/video") + data.source + "_" + std::to_string(data.timestamp) + ".mp4"), cv::VideoWriter::fourcc('m', 'p', '4', 'v'), 15.,
		    cv::Size(1920, 1200), {cv::VIDEOWRITER_PROP_HW_ACCELERATION, cv::VIDEO_ACCELERATION_NONE});
	}

	video.write(data.image);
}
