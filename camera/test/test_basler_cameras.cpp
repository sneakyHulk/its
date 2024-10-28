#include "BaslerCamerasNode.h"
#include "ImageVisualizationNode.h"
#include "PreprocessingNode.h"

int main() {
	BaslerCamerasNode cameras({{"s110_s_cam_8", {"0030532A9B7F"}}, {"s110_o_cam_8", {"003053305C72"}}, {"s110_n_cam_8", {"003053305C75"}}, {"s110_w_cam_8", {"003053380639"}}});
	PreprocessingNode pre({{"s110_n_cam_8", {1200, 1920, cv::ColorConversionCodes::COLOR_BayerBG2BGR}}, {"s110_w_cam_8", {1200, 1920, cv::ColorConversionCodes::COLOR_BayerBG2BGR}},
	    {"s110_s_cam_8", {1200, 1920, cv::ColorConversionCodes::COLOR_BayerBG2BGR}}, {"s110_o_cam_8", {1200, 1920, cv::ColorConversionCodes::COLOR_BayerBG2BGR}}});
	ImageVisualizationNode img([](ImageData const& data) { return data.source == "s110_s_cam_8"; });

	cameras += pre;
	pre += img;

	std::thread cameras_thread(&BaslerCamerasNode::operator(), &cameras);
	std::thread pre_thread(&PreprocessingNode::operator(), &pre);
	std::thread raw_img_thread(&ImageVisualizationNode::operator(), &img);
	std::this_thread::sleep_for(10s);
}