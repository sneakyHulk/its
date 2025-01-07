#pragma once

#include <opencv2/opencv.hpp>

#include "Detection2D.h"
#include "common_output.h"

template <int height, int width, int device_id = 0>
std::vector<Detection2D> run_yolo(cv::Mat const& downscaled_image, int camera_height = height, int camera_width = width, std::filesystem::path const& model_path = std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "yolo" / (std::to_string(height) + 'x' + std::to_string(width)) / "yolov9c.torchscript");