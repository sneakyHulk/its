#pragma once

#include <chrono>
#include <filesystem>
#include <map>

#include "ImageData.h"
#include "Runner.h"

/**
 * @class ImagePreprocessingNode
 * @brief This class saves image data to files.
 */
class ImageSavingNode : public Runner<ImageData> {
   public:
	struct FolderConfig {
		std::filesystem::path folder;
	};

	explicit ImageSavingNode(std::map<std::string, FolderConfig>&& config);

   private:
	void run(ImageData const& data) final;

	std::map<std::string, FolderConfig> _camera_name_folder_map;
};