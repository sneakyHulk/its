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

class PylonRAII {
   public:
	PylonRAII() { Pylon::PylonInitialize(); }
	~PylonRAII() { Pylon::PylonTerminate(); }
};

class Camera : public InputNode<ImageData> {
	// Before using any pylon methods, the pylon runtime must be initialized.
	static PylonRAII pylon_raii;

	bool controller_mode;
	Pylon::CBaslerUniversalInstantCamera camera;

   public:
	Camera();

   private:
	ImageData input_function() final;
	void init_camera();
	static bool is_camera_in_controller_mode(Pylon::CBaslerUniversalInstantCamera& camera);
};
