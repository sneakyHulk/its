#pragma once

#include <Eigen/Eigen>
#include <opencv2/opencv.hpp>

#include "Config.h"

void draw_camera_fov(cv::Mat& view, Config const& config, std::string const& camera_name, Eigen::Matrix<double, 4, 4> const& affine_transformation_base_to_image_center);
cv::Mat& draw_camera_fov(cv::Mat& view, std::string const& camera_name, int image_width, int image_height, Eigen::Matrix<double, 4, 4> const& affine_transformation_base_to_image_center, Eigen::Matrix<double, 3, 3> const& KR_inv,
    Eigen::Matrix<double, 3, 1> const& translation_camera);

struct ProjectionMatrixWidthHeightConfig {
	Eigen::Matrix<double, 3, 4> projection_matrix;
	int width;
	int height;
};

auto draw_map(std::filesystem::path&& odr_map, Config const& config, std::string const& base_name = "s110_base", int display_width = 1920, int display_height = 1200, double scaling = 4.) -> std::tuple<cv::Mat, Eigen::Matrix<double, 4, 4>>;
auto draw_map(std::filesystem::path&& odr_map, int display_width, int display_height, double scaling, Eigen::Matrix<double, 4, 4> const& utm_to_base,
    std::map<std::string, ProjectionMatrixWidthHeightConfig>&& camera_name_projection_matrix_width_height_map) -> std::tuple<cv::Mat, Eigen::Matrix<double, 4, 4>>;