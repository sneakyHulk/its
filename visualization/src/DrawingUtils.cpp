#include "DrawingUtils.h"

#include <OpenDriveMap.h>
#include <common_output.h>

#include <algorithm>
#include <autodiff/reverse/var.hpp>
#include <autodiff/reverse/var/eigen.hpp>
#include <map>
#include <random>
#include <ranges>
#include <vector>

#include "EigenUtils.h"

#if !__cpp_lib_ranges_zip
#include <range/v3/view/drop.hpp>
#include <range/v3/view/take.hpp>
#include <range/v3/view/zip.hpp>
#endif

template <typename scalar, int... other>
[[nodiscard]] Eigen::Matrix<scalar, 4, 1, other...> map_image_to_world_coordinate(std::array<scalar, 2> coordinates, Eigen::Matrix<double, 3, 3> const& KR_inv, Eigen::Matrix<double, 3, 1> const& translation_camera, scalar height = 0.) {
	Eigen::Matrix<scalar, 4, 1, other...> image_coordinates = Eigen::Matrix<scalar, 4, 1, other...>::Ones();
	image_coordinates(0) = coordinates[0];
	image_coordinates(1) = coordinates[1];

	image_coordinates.template head<3>() = KR_inv * image_coordinates.template head<3>();
	image_coordinates.template head<3>() = image_coordinates.template head<3>() * (height - translation_camera(2)) / image_coordinates(2);
	image_coordinates.template head<3>() = image_coordinates.template head<3>() + translation_camera;

	return image_coordinates;
}

cv::Mat& draw_camera_fov(cv::Mat& view, std::string const& camera_name, int const image_width, int const image_height, Eigen::Matrix<double, 4, 4> const& affine_transformation_base_to_image_center, Eigen::Matrix<double, 3, 3> const& KR_inv,
    Eigen::Matrix<double, 3, 1> const& translation_camera) {
	Eigen::Matrix<double, 4, 1> const left_bottom = affine_transformation_base_to_image_center * map_image_to_world_coordinate<double>({0., static_cast<double>(image_height)}, KR_inv, translation_camera);
	Eigen::Matrix<double, 4, 1> const right_bottom = affine_transformation_base_to_image_center * map_image_to_world_coordinate<double>({static_cast<double>(image_width), static_cast<double>(image_height)}, KR_inv, translation_camera);

	// The padding of the top y coordinators is necessary because if the camera points to the sky, it is no longer possible to compute a 2D point from it (singularity).
	// Testing different paddings:

	int left_padding = 0;
	{
		autodiff::var y = 0;
		autodiff::Vector4var left_top = affine_transformation_base_to_image_center * map_image_to_world_coordinate<autodiff::var>({0., y}, KR_inv, translation_camera);

		std::vector<double> derivatives;
		for (auto padding = 0; padding < image_height; ++padding) {
			y.update(padding);
			left_top(1).update();
			auto [dy] = autodiff::derivatives(left_top(1), autodiff::wrt(y));
			derivatives.push_back(dy);
		}
		left_padding = static_cast<int>(std::distance(derivatives.begin(), std::max_element(derivatives.begin(), derivatives.end(), [](double a, double b) { return std::fabs(a) < std::fabs(b); })));
		left_padding *= 2;  // more distance to the singularity
	}

	int right_padding = 0;
	{
		autodiff::var y = 0;
		autodiff::Vector4var right_top = affine_transformation_base_to_image_center * map_image_to_world_coordinate<autodiff::var>({image_width, y}, KR_inv, translation_camera);

		std::vector<double> derivatives;
		for (auto padding = 0; padding < image_height; ++padding) {
			y.update(padding);
			right_top(1).update();
			auto [dy] = autodiff::derivatives(right_top(1), autodiff::wrt(y));
			derivatives.push_back(dy);
		}
		right_padding = static_cast<int>(std::distance(derivatives.begin(), std::max_element(derivatives.begin(), derivatives.end(), [](double const a, double const b) { return std::fabs(a) < std::fabs(b); })));
		right_padding *= 2;  // more distance to the singularity
	}

	common::println(camera_name, ": left_padding: ", left_padding, ", right_padding: ", right_padding);

	Eigen::Matrix<double, 4, 1> const left_top = affine_transformation_base_to_image_center * map_image_to_world_coordinate<double>({0., static_cast<double>(left_padding)}, KR_inv, translation_camera);
	Eigen::Matrix<double, 4, 1> const right_top = affine_transformation_base_to_image_center * map_image_to_world_coordinate<double>({static_cast<double>(image_width), static_cast<double>(right_padding)}, KR_inv, translation_camera);

	auto overlay = view.clone();

	std::vector<cv::Point> points = {cv::Point(static_cast<int>(left_bottom(0, 0)), static_cast<int>(left_bottom(1, 0))), cv::Point(static_cast<int>(left_top(0, 0)), static_cast<int>(left_top(1, 0))),
	    cv::Point(static_cast<int>(right_top(0, 0)), static_cast<int>(right_top(1, 0))), cv::Point(static_cast<int>(right_bottom(0, 0)), static_cast<int>(right_bottom(1, 0)))};

	std::seed_seq seq(camera_name.begin(), camera_name.end());
	std::mt19937 rng(seq);
	std::uniform_int_distribution<> dis(0, 255);

	cv::fillConvexPoly(overlay, points, cv::Scalar(dis(rng), dis(rng), dis(rng)));
	cv::line(overlay, cv::Point(static_cast<int>(left_bottom(0, 0)), static_cast<int>(left_bottom(1, 0))), cv::Point(static_cast<int>(left_top(0, 0)), static_cast<int>(left_top(1, 0))), cv::Scalar(0, 0, 0), 1);
	cv::line(overlay, cv::Point(static_cast<int>(left_top(0, 0)), static_cast<int>(left_top(1, 0))), cv::Point(static_cast<int>(right_top(0, 0)), static_cast<int>(right_top(1, 0))), cv::Scalar(0, 0, 0), 1);
	cv::line(overlay, cv::Point(static_cast<int>(right_top(0, 0)), static_cast<int>(right_top(1, 0))), cv::Point(static_cast<int>(right_bottom(0, 0)), static_cast<int>(right_bottom(1, 0))), cv::Scalar(0, 0, 0), 1);
	cv::line(overlay, cv::Point(static_cast<int>(right_bottom(0, 0)), static_cast<int>(right_bottom(1, 0))), cv::Point(static_cast<int>(left_bottom(0, 0)), static_cast<int>(left_bottom(1, 0))), cv::Scalar(0, 0, 0), 1);

	auto alpha = 0.2;
	cv::addWeighted(view, alpha, overlay, 1 - alpha, 0, view);

	return view;
}

// base is point where image center should be located
auto draw_map(std::filesystem::path&& odr_map, int const display_width, int const display_height, double const scaling, Eigen::Matrix<double, 4, 4> const& utm_to_base,
    std::map<std::string, ProjectionMatrixWidthHeightConfig>&& camera_name_projection_matrix_width_height_map) -> std::tuple<cv::Mat, Eigen::Matrix<double, 4, 4>> {
	// Eigen::Matrix<double, 4, 4> const affine_transformation_rotate_90 = make_matrix<4, 4>(0., -1., 0., 0., 1., 0., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1.);
	Eigen::Matrix<double, 4, 4> const affine_transformation_reflect = make_matrix<4, 4>(-1., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1.);
	Eigen::Matrix<double, 4, 4> const affine_transformation_base_to_image_center =
	    make_matrix<4, 4>(1. * scaling, 0., 0., display_width / 2., 0., 1. * scaling, 0., display_height / 2., 0., 0., 1. * scaling, 0., 0., 0., 0., 1.) /* * affine_transformation_rotate_90*/ * affine_transformation_reflect;
	Eigen::Matrix<double, 4, 4> const affine_transformation_utm_to_image_center = affine_transformation_base_to_image_center * utm_to_base;

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

	for (auto const& [camera_name, config] : camera_name_projection_matrix_width_height_map) {
		Eigen::Matrix<double, 3, 3> KR = config.projection_matrix(Eigen::all, Eigen::seq(0, Eigen::last - 1));
		Eigen::Matrix<double, 3, 3> KR_inv = KR.inverse();
		Eigen::Matrix<double, 3, 1> C = config.projection_matrix(Eigen::all, Eigen::last);
		Eigen::Matrix<double, 3, 1> translation_camera = -KR_inv * C;

		draw_camera_fov(view, camera_name, config.height, config.width, affine_transformation_base_to_image_center, KR_inv, translation_camera);
	}

	return {view.clone(), affine_transformation_utm_to_image_center};
}