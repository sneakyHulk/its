#pragma once
#include <common_output.h>

#include <Eigen/Eigen>
#include <boost/geometry.hpp>
#include <boost/geometry/algorithms/is_convex.hpp>
#include <boost/geometry/geometries/ring.hpp>

namespace boost::geometry::traits {
	template <>
	struct tag<Eigen::Matrix<double, 4, 1>> {
		typedef point_tag type;
	};

	template <>
	struct coordinate_type<Eigen::Matrix<double, 4, 1>> {
		typedef double type;
	};

	template <>
	struct coordinate_system<Eigen::Matrix<double, 4, 1>> {
		typedef cs::cartesian type;
	};

	template <>
	struct dimension<Eigen::Matrix<double, 4, 1>> : boost::mpl::int_<2> {};

	template <>
	struct access<Eigen::Matrix<double, 4, 1>, 0> {
		static double get(Eigen::Matrix<double, 4, 1> const& p) { return p(0); }
		static void set(Eigen::Matrix<double, 4, 1>& p, double const& value) { p(0) = value; }
	};

	template <>
	struct access<Eigen::Matrix<double, 4, 1>, 1> {
		static double get(Eigen::Matrix<double, 4, 1> const& p) { return p(1); }
		static void set(Eigen::Matrix<double, 4, 1>& p, double const& value) { p(1) = value; }
	};
}  // namespace boost::geometry::traits

auto padding(auto const& affine_transformation_base_to_image_center, auto const& left_bottom, auto const& right_bottom, auto const& config, auto const& camera_name) {
	auto padding = 0;
	while (true) {
		Eigen::Matrix<double, 4, 1> const left_top = affine_transformation_base_to_image_center * config.camera_config.at(camera_name).image_to_world(0., padding, 0.);
		Eigen::Matrix<double, 4, 1> const right_top = affine_transformation_base_to_image_center * config.camera_config.at(camera_name).image_to_world(config.camera_config.at(camera_name).image_width, padding, 0.);

		boost::geometry::model::ring<Eigen::Matrix<double, 4, 1>> r;
		boost::geometry::append(r, left_bottom);
		boost::geometry::append(r, left_top);
		boost::geometry::append(r, right_top);
		boost::geometry::append(r, right_bottom);

		if (boost::geometry::is_convex(r)) {
			padding *= 1.5;  // to be further away from the singularities
			break;
		}

		if (!padding) {
			common::println("Camera '", camera_name, "' has pixel which lay in the sky. Computation of a 2D point with these is impossible!");
		}

		++padding;
	}
}