#pragma once

#include <exception>
#include <fstream>
#include <opencv2/opencv.hpp>

struct LensConfig {
	cv::Mat const _camera_matrix;
	cv::Mat const _distortion_values;

	LensConfig(cv::Mat&& camera_matrix, cv::Mat&& distortion_values) : _camera_matrix(std::forward<decltype(camera_matrix)>(camera_matrix)), _distortion_values(std::forward<decltype(distortion_values)>(distortion_values)) {}

	[[nodiscard]] cv::Mat const& camera_matrix() const { return _camera_matrix; }
	[[nodiscard]] cv::Mat const& distortion_values() const { return _distortion_values; }
};

LensConfig make_lens_config(std::istream& config);