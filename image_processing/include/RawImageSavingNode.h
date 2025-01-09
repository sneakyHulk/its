#pragma once

#include <chrono>
#include <filesystem>
#include <map>

#include "ImageDataRaw.h"
#include "Runner.h"

/**
 * @class ImagePreprocessingNode
 * @brief This class saves raw image data to files.
 */
class RawImageSavingNode : public Runner<ImageDataRaw> {
   public:
	struct FolderConfig {
		std::filesystem::path folder;
	};

	explicit RawImageSavingNode(std::map<std::string, FolderConfig>&& config);

   private:
	void run(ImageDataRaw const& data) final;

	std::map<std::string, FolderConfig> _camera_name_folder_map;
};