#pragma once

#include <OpenDriveMap.h>
#include <common_output.h>

#include <Eigen/Eigen>
#include <algorithm>
#include <autodiff/reverse/var.hpp>
#include <autodiff/reverse/var/eigen.hpp>
#include <opencv2/opencv.hpp>
#include <random>
#include <ranges>
#include <vector>

#include "Config.h"

#if !__cpp_lib_ranges_zip
#include <range/v3/view/zip.hpp>
#include <range/v3/view/take.hpp>
#include <range/v3/view/drop.hpp>
#endif

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dis(0, 255);

void draw_camera_fov(cv::Mat& view, Config const& config, std::string const& camera_name, Eigen::Matrix<double, 4, 4> const& affine_transformation_base_to_image_center) {
	std::seed_seq seq(camera_name.begin(), camera_name.end());
	std::mt19937 rng(seq);
	std::uniform_int_distribution<> dis(0, 255);

	Eigen::Matrix<double, 4, 1> const left_bottom = affine_transformation_base_to_image_center * config.map_image_to_world_coordinate<double>(camera_name, 0., config.camera_config(camera_name).image_height(), 0.);
	Eigen::Matrix<double, 4, 1> const right_bottom =
	    affine_transformation_base_to_image_center * config.map_image_to_world_coordinate<double>(camera_name, config.camera_config(camera_name).image_width(), config.camera_config(camera_name).image_height(), 0.);

	// The padding of the top y coordinators is necessary because if the camera points to the sky, it is no longer possible to compute a 2D point from it (singularity).
	// Testing different paddings:

	int left_padding = 0;
	{
		autodiff::var y = 0;
		autodiff::Vector4var left_top = affine_transformation_base_to_image_center * config.map_image_to_world_coordinate<autodiff::var>(camera_name, 0., y, 0.);

		std::vector<double> derivates;
		for (auto padding = 0; padding < config.camera_config(camera_name).image_height(); ++padding) {
			y.update(padding);
			left_top(1).update();
			auto [dy] = autodiff::derivatives(left_top(1), autodiff::wrt(y));
			derivates.push_back(dy);
		}
		left_padding = static_cast<int>(std::distance(derivates.begin(), std::max_element(derivates.begin(), derivates.end(), [](double a, double b) { return std::fabs(a) < std::fabs(b); })));
		left_padding *= 2;  // more distance to the singularity
	}

	int right_padding = 0;
	{
		autodiff::var y = 0;
		autodiff::Vector4var right_top = affine_transformation_base_to_image_center * config.map_image_to_world_coordinate<autodiff::var>(camera_name, config.camera_config(camera_name).image_width(), y, 0.);

		std::vector<double> derivates;
		for (auto padding = 0; padding < config.camera_config(camera_name).image_height(); ++padding) {
			y.update(padding);
			right_top(1).update();
			auto [dy] = autodiff::derivatives(right_top(1), autodiff::wrt(y));
			derivates.push_back(dy);
		}
		right_padding = static_cast<int>(std::distance(derivates.begin(), std::max_element(derivates.begin(), derivates.end(), [](double const a, double const b) { return std::fabs(a) < std::fabs(b); })));
		right_padding *= 2;  // more distance to the singularity
	}

	common::println(camera_name, ": left_padding: ", left_padding, ", right_padding: ", right_padding);

	// Eigen::Matrix<double, 4, 1> const left_top = affine_transformation_base_to_image_center * config.camera_config.at(camera_name).image_to_world(0., left_padding, 0.);
	Eigen::Matrix<double, 4, 1> const left_top = affine_transformation_base_to_image_center * config.map_image_to_world_coordinate<double>(camera_name, 0., left_padding, 0.);
	// Eigen::Matrix<double, 4, 1> const right_top = affine_transformation_base_to_image_center * config.camera_config.at(camera_name).image_to_world(config.camera_config.at(camera_name).image_width, right_padding, 0.);
	Eigen::Matrix<double, 4, 1> const right_top = affine_transformation_base_to_image_center * config.map_image_to_world_coordinate<double>(camera_name, config.camera_config(camera_name).image_width(), right_padding, 0.);

	auto overlay = view.clone();

	std::vector<cv::Point> points = {cv::Point(static_cast<int>(left_bottom(0, 0)), static_cast<int>(left_bottom(1, 0))), cv::Point(static_cast<int>(left_top(0, 0)), static_cast<int>(left_top(1, 0))),
	    cv::Point(static_cast<int>(right_top(0, 0)), static_cast<int>(right_top(1, 0))), cv::Point(static_cast<int>(right_bottom(0, 0)), static_cast<int>(right_bottom(1, 0)))};

	cv::fillConvexPoly(overlay, points, cv::Scalar(dis(rng), dis(rng), dis(rng)));
	cv::line(overlay, cv::Point(static_cast<int>(left_bottom(0, 0)), static_cast<int>(left_bottom(1, 0))), cv::Point(static_cast<int>(left_top(0, 0)), static_cast<int>(left_top(1, 0))), cv::Scalar(0, 0, 0), 1);
	cv::line(overlay, cv::Point(static_cast<int>(left_top(0, 0)), static_cast<int>(left_top(1, 0))), cv::Point(static_cast<int>(right_top(0, 0)), static_cast<int>(right_top(1, 0))), cv::Scalar(0, 0, 0), 1);
	cv::line(overlay, cv::Point(static_cast<int>(right_top(0, 0)), static_cast<int>(right_top(1, 0))), cv::Point(static_cast<int>(right_bottom(0, 0)), static_cast<int>(right_bottom(1, 0))), cv::Scalar(0, 0, 0), 1);
	cv::line(overlay, cv::Point(static_cast<int>(right_bottom(0, 0)), static_cast<int>(right_bottom(1, 0))), cv::Point(static_cast<int>(left_bottom(0, 0)), static_cast<int>(left_bottom(1, 0))), cv::Scalar(0, 0, 0), 1);

	auto alpha = 0.2;
	cv::addWeighted(view, alpha, overlay, 1 - alpha, 0, view);
}

auto draw_map(std::filesystem::path odr_map, Config const& config, std::string const base_name = "s110_base", int display_width = 1920, int display_height = 1200, double scaling = 4.) -> std::tuple<cv::Mat, Eigen::Matrix<double, 4, 4>> {
	Eigen::Matrix<double, 4, 4> const affine_transformation_rotate_90 = make_matrix<4, 4>(0., -1., 0., 0., 1., 0., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1.);
	Eigen::Matrix<double, 4, 4> const affine_transformation_reflect = make_matrix<4, 4>(-1., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1.);
	Eigen::Matrix<double, 4, 4> const affine_transformation_base_to_image_center =
	    make_matrix<4, 4>(1. * scaling, 0., 0., display_width / 2., 0., 1. * scaling, 0., display_height / 2., 0., 0., 1. * scaling, 0., 0., 0., 0., 1.) * affine_transformation_rotate_90 * affine_transformation_reflect;
	Eigen::Matrix<double, 4, 4> const affine_transformation_utm_to_image_center = affine_transformation_base_to_image_center * config.affine_transformation_map_origin_to_bases(base_name) * config.affine_transformation_utm_to_map_origin();

	cv::Mat view(display_height, display_width, CV_8UC3, cv::Scalar(255, 255, 255));  // Declaring a white matrix

	odr::OpenDriveMap garching(odr_map);

	for (odr::Road const& road : garching.get_roads()) {
		for (odr::LaneSection const& lanesection : road.get_lanesections()) {
			for (odr::Lane const& lane : lanesection.get_lanes()) {
				auto lane_border = road.get_lane_border_line(lane, 0.1);

				cv::Scalar black_color(0, 0, 0);

#if __cpp_lib_ranges_zip
				auto adjacent_range = lane_border | std::ranges::views::adjacent<2>;
#else
				auto adjacent_range = ranges::view::zip(lane_border | ranges::view::take(lane_border.size() - 1), lane_border | ranges::view::drop(1));
#endif

				for (auto const [start, end] : adjacent_range) {
					Eigen::Vector4d start_;
					start_ << start[0], start[1], start[2], 1.;
					Eigen::Vector4d end_;
					end_ << end[0], end[1], end[2], 1.;

					auto start_transformed = affine_transformation_utm_to_image_center * start_;
					auto end_transformed = affine_transformation_utm_to_image_center * end_;

					cv::line(view, cv::Point(static_cast<int>(start_transformed(0)), static_cast<int>(start_transformed(1))), cv::Point(static_cast<int>(end_transformed(0)), static_cast<int>(end_transformed(1))), black_color, 1);
				}
			}
		}
	}

	auto tmp = affine_transformation_base_to_image_center * make_matrix<4, 1>(0., 0., 0., 1.);
	cv::Scalar other_color(0, 0, 255);
	cv::circle(view, cv::Point(static_cast<int>(tmp(0)), static_cast<int>(tmp(1))), 1, other_color, 5);

	draw_camera_fov(view, config, "s110_s_cam_8", affine_transformation_base_to_image_center);
	draw_camera_fov(view, config, "s110_n_cam_8", affine_transformation_base_to_image_center);
	draw_camera_fov(view, config, "s110_o_cam_8", affine_transformation_base_to_image_center);
	draw_camera_fov(view, config, "s110_w_cam_8", affine_transformation_base_to_image_center);

	return {view.clone(), affine_transformation_utm_to_image_center};
}