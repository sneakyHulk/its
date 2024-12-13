#pragma once

#include <filesystem>
#include <opencv2/opencv.hpp>
#include <vector>

#include "common_output.h"
#include "node.h"
#include "ImageData.h"


class CameraSimulator : public InputNode<ImageData> {
	std::vector<std::filesystem::directory_entry> files;
	std::vector<std::filesystem::directory_entry> files_copy;

   public:
	explicit CameraSimulator(std::string const& cam_name);

   private:
	ImageData input_function() final;
};