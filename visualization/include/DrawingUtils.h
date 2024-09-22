#pragma once

#include <Eigen/Eigen>
#include <opencv2/opencv.hpp>

#include "Config.h"

void draw_camera_fov(cv::Mat& view, Config const& config, std::string const& camera_name, Eigen::Matrix<double, 4, 4> const& affine_transformation_base_to_image_center);

auto draw_map(std::filesystem::path&& odr_map, Config const& config, const std::string& base_name = "s110_base", int display_width = 1920, int display_height = 1200, double scaling = 4.) -> std::tuple<cv::Mat, Eigen::Matrix<double, 4, 4>>;