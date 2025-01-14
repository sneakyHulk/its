#include <boost/circular_buffer.hpp>
#include <filesystem>

#include "VideoVisualizationNode.h"
#include "camera_node.h"

class PylonRAII {
   public:
	PylonRAII() { Pylon::PylonInitialize(); }
	~PylonRAII() { Pylon::PylonTerminate(); }
};

int main(int argc, char* argv[]) {
	// Before using any pylon methods, the pylon runtime must be initialized.
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

			std::string time(std::to_string(ptrGrabResult->GetTimeStamp()));

			if (cv::imwrite(std::filesystem::path(CMAKE_SOURCE_DIR) / std::filesystem::path(std::string("result/img_") + std::to_string(i) + ".png"), image)) {
			} else {
				common::println("Problem with imwrite!");
			}
		}
	}

	{
		Camera cam_s("0030532A9B7F", "s110_s_cam_8");
		Camera cam_o("003053305C72", "s110_o_cam_8");
		Camera cam_n("003053305C75", "s110_n_cam_8");
		// Camera cam_w("003053380639", "s110_w_cam_8");

		VideoVisualization vid_s;
		VideoVisualization vid_o;
		VideoVisualization vid_n;
		VideoVisualization vid_w;

		cam_s += vid_s;
		cam_o += vid_o;
		cam_n += vid_n;
		// cam_w += vid_w;

		std::thread cam_s_thread(&Camera::operator(), &cam_s);
		std::thread cam_o_thread(&Camera::operator(), &cam_o);
		std::thread cam_n_thread(&Camera::operator(), &cam_n);
		// std::thread cam_w_thread(&Camera::operator(), &cam_w);

		std::thread vid_s_thread(&VideoVisualization::operator(), &vid_s);
		std::thread vid_o_thread(&VideoVisualization::operator(), &vid_o);
		std::thread vid_n_thread(&VideoVisualization::operator(), &vid_n);
		// std::thread vid_w_thread(&VideoVisualization::operator(), &vid_w);

		std::this_thread::sleep_for(100000s);
	}
}