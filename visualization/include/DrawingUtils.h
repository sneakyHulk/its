#pragma once

#include <Eigen/Eigen>
#include <filesystem>
#include <opencv2/opencv.hpp>

/**
 * @brief Draws the camera fovs into the bird's eye view image.
 * @param view The bird's eye view image.
 * @param camera_name The name of the camera whose FOV is to be displayed.
 * @param image_height The height of the camera whose FOV is to be displayed.
 * @param image_width The width of the camera whose FOV is to be displayed.
 * @param affine_transformation_base_to_image_center The affine transformation matrix for the transition from the center point of the OpenDrive map to the center of the bird's eye view image.
 * @return The bird eye view image.
 */
cv::Mat& draw_camera_fov(cv::Mat& view, std::string const& camera_name, int image_height, int image_width, Eigen::Matrix<double, 4, 4> const& affine_transformation_base_to_image_center, Eigen::Matrix<double, 3, 3> const& KR_inv,
    Eigen::Matrix<double, 3, 1> const& translation_camera);

struct ProjectionMatrixWidthHeightConfig {
	Eigen::Matrix<double, 3, 4> projection_matrix;  // Projection Matrix form camera to base specified in draw_map
	int height;
	int width;
};

/**
 * @brief Draws a bird's eye view image from an OpenDrive map.
 * @param odr_map The filepath of the OpenDrive map.
 * @param display_height The height of the resulting map.
 * @param display_width The width of the resulting map.
 * @param scaling The zoom level of the resulting map, with larger numbers representing more zoomed-in maps.
 * @param utm_to_base The affine transformation matrix for the transition from the UTM coordinate system to the center point of the OpenDrive map.
 * @param camera_name_projection_matrix_width_height_map The map that maps the cameras to their projection matrix, height and width. Is needed to display the different camera fovs.
 * @return The bird eye view image.
 */
auto draw_map(std::filesystem::path&& odr_map, int display_height, int display_width, double scaling, Eigen::Matrix<double, 4, 4> const& utm_to_base,
    std::map<std::string, ProjectionMatrixWidthHeightConfig>&& camera_name_projection_matrix_width_height_map) -> std::tuple<cv::Mat, Eigen::Matrix<double, 4, 4>>;