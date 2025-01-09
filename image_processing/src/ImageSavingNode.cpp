#include "ImageSavingNode.h"

#include <opencv2/opencv.hpp>

ImageSavingNode::ImageSavingNode(std::map<std::string, FolderConfig>&& config) : _camera_name_folder_map(std::forward<decltype(config)>(config)) {
	for (auto const& [camera_name, folder_config] : _camera_name_folder_map) {
		std::filesystem::create_directories(folder_config.folder);
	}
}

/**
 * @brief Saves the incoming image data to the previous defined folder.
 *
 * @param data The image data to be saved.
 */
void ImageSavingNode::run(const ImageData& data) {
	cv::imwrite(
	    _camera_name_folder_map.at(data.source).folder / (std::to_string(std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now()).time_since_epoch().count()) + '_' + std::to_string(data.timestamp) + ".jpg"),
	    data.image);
}
