#include "LensConfig.h"

LensConfig make_lens_config(std::istream& config) {
	cv::Mat camera_matrix = cv::Mat_<double>(3, 3);
	cv::Mat distortion_values;

	std::string name;
	config >> name;
	if (name != "intrinsic") throw std::invalid_argument("wrong file format");

	for (auto i = 0; i < camera_matrix.rows; ++i) {
		for (auto j = 0; j < camera_matrix.cols; ++j) {
			config >> camera_matrix.at<double>(i, j);
		}
	}

	config >> name;
	if (name != "distortion") throw std::invalid_argument("wrong file format");
	for (double x = 0.0; config >> x; distortion_values.push_back(x))
		;

	return LensConfig{std::forward<decltype(camera_matrix)>(camera_matrix), std::forward<decltype(distortion_values)>(distortion_values)};
}