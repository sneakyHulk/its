#pragma once
#include <Eigen/Eigen>
#include <concepts>
#include <filesystem>
#include <fstream>
#include <map>
#include <nlohmann/json.hpp>
#include <regex>
#include <span>
#include <utility>

#include "CameraConfig.h"
#include "EigenUtils.h"
#include "LensConfig.h"

struct Config {
	Eigen::Matrix<double, 4, 4> const _affine_transformation_map_origin_to_utm;
	Eigen::Matrix<double, 4, 4> const _affine_transformation_utm_to_map_origin;
	std::map<std::string, Eigen::Matrix<double, 4, 4>> const _affine_transformation_map_origin_to_bases;
	std::map<std::string, Eigen::Matrix<double, 4, 4>> const _affine_transformation_bases_to_map_origin;

	std::map<std::string, LensConfig> const _lens_config;
	std::map<std::tuple<std::string, int, int>, cv::Mat> const _new_camera_matrix;

	std::map<std::string, CameraConfig> const _camera_config;

	Config(Eigen::Matrix<double, 4, 4>&& affine_transformation_map_origin_to_utm, Eigen::Matrix<double, 4, 4>&& affine_transformation_utm_to_map_origin,
	    std::map<std::string, Eigen::Matrix<double, 4, 4>>&& affine_transformation_map_origin_to_bases, std::map<std::string, Eigen::Matrix<double, 4, 4>>&& affine_transformation_bases_to_map_origin,
	    std::map<std::string, CameraConfig>&& camera_config, std::map<std::string, LensConfig>&& lens_config, std::map<std::tuple<std::string, int, int>, cv::Mat>&& new_camera_matrix)
	    : _affine_transformation_map_origin_to_utm(std::forward<decltype(affine_transformation_map_origin_to_utm)>(affine_transformation_map_origin_to_utm)),
	      _affine_transformation_utm_to_map_origin(std::forward<decltype(affine_transformation_utm_to_map_origin)>(affine_transformation_utm_to_map_origin)),
	      _affine_transformation_map_origin_to_bases(std::forward<decltype(affine_transformation_map_origin_to_bases)>(affine_transformation_map_origin_to_bases)),
	      _affine_transformation_bases_to_map_origin(std::forward<decltype(affine_transformation_bases_to_map_origin)>(affine_transformation_bases_to_map_origin)),
	      _camera_config(std::forward<decltype(camera_config)>(camera_config)),
	      _lens_config(std::forward<decltype(lens_config)>(lens_config)),
	      _new_camera_matrix(std::forward<decltype(new_camera_matrix)>(new_camera_matrix)) {}

	template <typename scalar, int... other>
	[[nodiscard]] Eigen::Matrix<scalar, 4, 1, other...> map_image_to_world_coordinate(std::string const& camera_name, scalar x, scalar y, scalar height) const {
		Eigen::Matrix<scalar, 4, 1, other...> image_coordinates = Eigen::Matrix<scalar, 4, 1, other...>::Ones();
		image_coordinates(0) = x;
		image_coordinates(1) = y;

		image_coordinates.template head<3>() = camera_config(camera_name).KR_inv * image_coordinates.template head<3>();
		image_coordinates.template head<3>() = image_coordinates.template head<3>() * (height - camera_config(camera_name).translation_camera(2)) / image_coordinates(2);
		image_coordinates.template head<3>() = image_coordinates.template head<3>() + camera_config(camera_name).translation_camera;

		return image_coordinates;
	}

	[[nodiscard]] Eigen::Matrix<double, 4, 4> const& affine_transformation_map_origin_to_utm() const { return _affine_transformation_map_origin_to_utm; }
	[[nodiscard]] Eigen::Matrix<double, 4, 4> const& affine_transformation_utm_to_map_origin() const { return _affine_transformation_utm_to_map_origin; }
	[[nodiscard]] Eigen::Matrix<double, 4, 4> const& affine_transformation_map_origin_to_bases(std::string const& camera_name) const { return _affine_transformation_map_origin_to_bases.at(camera_name); }
	[[nodiscard]] Eigen::Matrix<double, 4, 4> const& affine_transformation_bases_to_map_origin(std::string const& camera_name) const { return _affine_transformation_bases_to_map_origin.at(camera_name); }

	[[nodiscard]] LensConfig const& lens_config(std::string const& camera_name) const { return _lens_config.at(camera_config(camera_name).lens_name()); }
	[[nodiscard]] CameraConfig const& camera_config(std::string const& camera_name) const { return _camera_config.at(camera_name); }
	[[nodiscard]] cv::Mat const& new_camera_matrix(std::string const& camera_name) const {
		return _new_camera_matrix.at(std::tie(camera_config(camera_name).lens_name(), camera_config(camera_name).image_width(), camera_config(camera_name).image_height()));
	}
	[[nodiscard]] std::vector<cv::Point2d> undistort_points(std::string const& camera_name, std::vector<cv::Point2d> const& distorted_points) const {
		std::vector<cv::Point2d> out;

		cv::undistortPoints(distorted_points, out, lens_config(camera_name).camera_matrix(), lens_config(camera_name).distortion_values(), cv::Mat_<double>::eye(3, 3), new_camera_matrix(camera_name));

		return out;
	}

	[[nodiscard]] std::tuple<double, double> undistort_point(std::string const& camera_name, double const x, double const y) const {
		std::vector<cv::Point2d> out(1);
		std::vector<cv::Point2d> distorted_point = {{x, y}};

		cv::undistortPoints(distorted_point, out, lens_config(camera_name).camera_matrix(), lens_config(camera_name).distortion_values(), cv::Mat_<double>::eye(3, 3), new_camera_matrix(camera_name));

		return {out.front().x, out.front().y};
	}
};

Config make_config(std::filesystem::path const config_directory = std::filesystem::path(CMAKE_SOURCE_DIR) / std::filesystem::path("thirdparty/projection_library/config"), double const map_origin_x = 695942.4856864865,
    double const map_origin_y = 5346521.128436302, double const map_origin_z = 485.0095881917835);