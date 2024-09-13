#pragma once
#include <pylon/GrabResultPtr.h>
#include <pylon/PylonIncludes.h>

#include <filesystem>
#include <opencv2/opencv.hpp>

#include "ImageData.h"
#include "common_exception.h"
#include "common_output.h"
#include "node.h"

class BaslerCameras : public InputNode<Pylon::CGrabResultPtr> {
	friend class BaslerSaveRAW;
	Pylon::CInstantCameraArray cameras;
	std::map<std::string, std::string> const mac_to_cam_name;
	std::vector<std::string> index_to_cam_name;

   public:
	explicit BaslerCameras(std::map<std::string, std::string> const& mac_and_cam_names = {{"0030532A9B7F", "s110_s_cam_8"}, {"003053305C72", "s110_o_cam_8"}, {"003053305C75", "s110_n_cam_8"}, {"003053380639", "s110_w_cam_8"}});
	Pylon::CGrabResultPtr input_function() final;
	void init_cameras();
};

class BaslerSaveRAW : public OutputNode<Pylon::CGrabResultPtr> {
	std::filesystem::path const folder;
	BaslerCameras const& cameras;

   public:
	explicit BaslerSaveRAW(BaslerCameras const& cameras, std::filesystem::path folder = std::filesystem::path(CMAKE_SOURCE_DIR) / "result");

	void output_function(Pylon::CGrabResultPtr const& data) final;
};