#include <pylon/BaslerUniversalInstantCamera.h>
#include <pylon/GrabResultPtr.h>
#include <pylon/PylonIncludes.h>

#include <filesystem>
#include <opencv2/opencv.hpp>

#include "common_exception.h"
#include "common_output.h"

class PylonRAII {
   public:
	PylonRAII() { Pylon::PylonInitialize(); }
	~PylonRAII() { Pylon::PylonTerminate(); }
};

int main(int argc, char* argv[]) {
	PylonRAII pylon_raii;

	{
		Pylon::DeviceInfoList device_list;
		Pylon::CTlFactory::GetInstance().EnumerateDevices(device_list);
		if (device_list.empty()) throw common::Exception("No Basler devices found!");

		for (auto i = 0; i < device_list.size(); ++i) {
			common::println("Found ", i, " device with model name '", device_list.at(i).GetModelName(), "', ip address '", device_list.at(i).GetIpAddress(), "', and mac address '", device_list.at(i).GetMacAddress(), "'.");

			Pylon::CBaslerUniversalInstantCamera camera;
			camera.Attach(Pylon::CTlFactory::GetInstance().CreateDevice(device_list.at(i)));

			camera.Open();
			camera.StartGrabbing();

			Pylon::CGrabResultPtr ptrGrabResult;
			camera.RetrieveResult(1000, ptrGrabResult);

			cv::Mat bayer_image(static_cast<int>(ptrGrabResult->GetHeight()), static_cast<int>(ptrGrabResult->GetWidth()), CV_8UC1, ptrGrabResult->GetBuffer());
			cv::Mat image;
			cv::cvtColor(bayer_image, image, cv::COLOR_BayerBG2BGR);

			if (cv::imwrite(std::filesystem::path(CMAKE_SOURCE_DIR) / std::filesystem::path(std::string("result/test2.png")), image)) {
			} else {
				common::println("Problem with imwrite!");
			}
		}
	}
}