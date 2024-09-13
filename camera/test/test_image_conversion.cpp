#include <cinttypes>
#include <filesystem>
#include <fstream>
#include <opencv2/opencv.hpp>

#include "ImageData.h"
#include "video_node.h"

class RAW2ImageData : public InputNode<ImageData> {
	cv::Mat image;

   public:
	explicit RAW2ImageData(std::filesystem::path const& path = std::filesystem::path(CMAKE_SOURCE_DIR) / std::filesystem::path("data/basler/image.raw")) {
		std::ifstream in(path, std::ios::binary);
		std::vector<std::uint8_t> image_raw((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
		in.close();

		cv::Mat bayer_image(1200, 1920, CV_8UC1, image_raw.data());
		cv::cvtColor(bayer_image, image, cv::COLOR_BayerBG2BGR);

		cv::imshow("image", image);
		cv::waitKey(0);
	}

   private:
	ImageData input_function() final {
		std::this_thread::sleep_for(100ms);

		common::println("[RAW]: image!");

		return ImageData(image, 1, 1920, 1200, "raw");
	}
};

int main(int argc, char* argv[]) {
	RAW2ImageData raw(std::filesystem::path(CMAKE_SOURCE_DIR) / std::filesystem::path("data/basler/1726238661751532982_1726238661401530635"));
	VideoVisualization vis;

	raw += vis;

	std::thread cam_s_thread(&RAW2ImageData::operator(), &raw);
	vis();
}