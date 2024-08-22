#pragma once

#include <pylon/BaslerUniversalInstantCamera.h>
#include <pylon/GrabResultPtr.h>
#include <pylon/ImageEventHandler.h>
#include <pylon/PylonIncludes.h>

#include <opencv2/opencv.hpp>

#include "ImageData.h"
#include "common_exception.h"
#include "common_output.h"
#include "node.h"

class Camera : public InputNode<ImageData> {
	bool controller_mode;
	Pylon::CBaslerUniversalInstantCamera camera;
	std::string const mac_adress;
	std::string const cam_name;

   public:
	Camera(std::string mac_address, std::string cam_name);

   private:
	ImageData input_function() final;
	void init_camera();
};
