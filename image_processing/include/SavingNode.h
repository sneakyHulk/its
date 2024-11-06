#pragma once

#include <chrono>
#include <filesystem>
#include <fstream>
#include <map>

#include "ImageData.h"
#include "ImageDataRaw.h"
#include "node.h"

class SavingImageDataNode : public Runner<ImageData> {
   public:
	struct FolderConfig {
		std::filesystem::path folder;
	};

	explicit SavingImageDataNode(std::map<std::string, FolderConfig>&& config) : _camera_name_folder_map(std::forward<decltype(config)>(config)) {
		for (auto const& [camera_name, folder_config] : _camera_name_folder_map) {
			std::filesystem::create_directories(folder_config.folder);
		}
	}

   private:
	void run(ImageData const& data) final {
		cv::imwrite(_camera_name_folder_map.at(data.source).folder /
		                (std::to_string(std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now()).time_since_epoch().count()) + '_' + std::to_string(data.timestamp) + ".jpg"),
		    data.image);
	}

	std::map<std::string, FolderConfig> _camera_name_folder_map;
};

class SavingImageDataRawNode : public Runner<ImageDataRaw> {
   public:
	struct FolderConfig {
		std::filesystem::path folder;
	};

	explicit SavingImageDataRawNode(std::map<std::string, FolderConfig>&& config) : _camera_name_folder_map(std::forward<decltype(config)>(config)) {
		for (auto const& [camera_name, folder_config] : _camera_name_folder_map) {
			std::filesystem::create_directories(folder_config.folder);
		}
	}

   private:
	void run(ImageDataRaw const& data) final {
		std::ofstream raw_image(
		    _camera_name_folder_map.at(data.source).folder / (std::to_string(std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now()).time_since_epoch().count()) + '_' + std::to_string(data.timestamp)));

		raw_image.write(reinterpret_cast<const char*>(data.image_raw.data()), data.image_raw.size());
	}

	std::map<std::string, FolderConfig> _camera_name_folder_map;
};