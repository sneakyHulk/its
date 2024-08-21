#include <cinttypes>
#include <filesystem>
#include <fstream>
#include <opencv2/opencv.hpp>

int main(int argc, char* argv[]) {
	std::ifstream in(std::filesystem::path(CMAKE_SOURCE_DIR) / std::filesystem::path("data/camera/image.raw"), std::ios::binary);
	std::vector<std::uint8_t> image_raw((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

	in.close();

	{
		cv::Mat bayer_image(1200, 1920, CV_8UC1, image_raw.data());
		cv::Mat image;
		cv::cvtColor(bayer_image, image, cv::COLOR_BayerBG2BGR);

		cv::imshow("image", image);

		cv::waitKey(0);
	}
}