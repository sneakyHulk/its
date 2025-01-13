#pragma once
#include <Eigen/Eigen>
#include <opencv2/opencv.hpp>

#include "EigenUtils.h"

namespace config {
	Eigen::Matrix<double, 4, 4> const affine_transformation_rotate_90 = make_matrix<4, 4>(0., -1., 0., 0., 1., 0., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1.);
	Eigen::Matrix<double, 4, 4> const affine_transformation_nothing = make_matrix<4, 4>(1., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1.);

	Eigen::Matrix<double, 4, 4> const affine_transformation_map_origin_to_utm = make_matrix<4, 4>(
#include "config/affine_transformation_map_origin_to_utm"
	);

	Eigen::Matrix<double, 4, 4> const affine_transformation_map_origin_to_s110_base = make_matrix<4, 4>(
#include "config/affine_transformation_map_origin_to_s110_base"
	);
	// s110_base is 90° rotated, so to make the s110_base
	Eigen::Matrix<double, 4, 4> const affine_transformation_utm_to_s110_base_north = affine_transformation_rotate_90.inverse() * affine_transformation_map_origin_to_s110_base * affine_transformation_map_origin_to_utm.inverse();

	Eigen::Matrix<double, 4, 4> const affine_transformation_s110_lidar_ouster_south_to_s110_base = make_matrix<4, 4>(
#include "config/affine_transformation_lidar_south_to_s110_base"
	);

	Eigen::Matrix<double, 4, 4> const affine_transformation_utm_to_s110_lidar_ouster_south =
	    affine_transformation_s110_lidar_ouster_south_to_s110_base.inverse() * affine_transformation_map_origin_to_s110_base * affine_transformation_map_origin_to_utm.inverse();

	// projection matrices
	Eigen::Matrix<double, 3, 4> const projection_matrix_s110_base_into_s110_n_cam_8 = make_matrix<3, 4>(
#include "config/projection_matrix_s110_n_cam_8"
	);
	Eigen::Matrix<double, 3, 4> const projection_matrix_s110_base_north_into_s110_n_cam_8 = projection_matrix_s110_base_into_s110_n_cam_8 * affine_transformation_rotate_90;

	Eigen::Matrix<double, 3, 4> const projection_matrix_s110_base_into_s110_o_cam_8 = make_matrix<3, 4>(
#include "config/projection_matrix_s110_o_cam_8"
	);
	Eigen::Matrix<double, 3, 4> const projection_matrix_s110_base_north_into_s110_o_cam_8 = projection_matrix_s110_base_into_s110_o_cam_8 * affine_transformation_rotate_90;

	Eigen::Matrix<double, 3, 4> const projection_matrix_s110_base_into_s110_s_cam_8 = make_matrix<3, 4>(
#include "config/projection_matrix_s110_s_cam_8"
	);
	Eigen::Matrix<double, 3, 4> const projection_matrix_s110_base_north_into_s110_s_cam_8 = projection_matrix_s110_base_into_s110_s_cam_8 * affine_transformation_rotate_90;

	Eigen::Matrix<double, 3, 4> const projection_matrix_s110_base_into_s110_w_cam_8 = make_matrix<3, 4>(
#include "config/projection_matrix_s110_w_cam_8"
	);
	[[maybe_unused]] Eigen::Matrix<double, 3, 4> const projection_matrix_s110_base_into_s110_w_cam_8_2 = make_matrix<3, 4>(1599.6787257016188, 391.55387236603775, -430.34650625835917, 6400.522155319611, -21.862527625533737,
	    -135.38146150648188, -1512.651893582593, 13030.4682633739, 0.27397972486181504, 0.842440925400074, -0.4639271468406554, 4.047780978836272);
	Eigen::Matrix<double, 3, 4> const projection_matrix_s110_base_north_into_s110_w_cam_8 = projection_matrix_s110_base_into_s110_w_cam_8 * affine_transformation_rotate_90;

	Eigen::Matrix<double, 3, 4> const projection_matrix_vehicle_lidar_robosense_into_vehicle_camera_basler_16mm = make_matrix<3, 4>(1019.929965441548, -2613.286262078907, 184.6794570200418, 370.7180273597151, 589.8963703919744,
	    -24.09642935106967, -2623.908527352794, -139.3143336725661, 0.9841844439506531, 0.1303769648075104, 0.1199281811714172, -0.1664766669273376);

	Eigen::Matrix<double, 3, 4> const projection_matrix_s110_lidar_ouster_south_into_s110_n_cam_8 =
	    make_matrix<3, 4>(-1.85236617e+02, -1.50398668e+03, -5.25918451e+02, -2.33348780e+04, -2.40170444e+02, 2.20604673e+02, -1.56725360e+03, 6.36103284e+03, 6.86399000e-01, -4.49337000e-01, -5.71798000e-01, -6.75018000e+00);
	Eigen::Matrix<double, 3, 4> const projection_matrix_s110_lidar_ouster_south_into_s110_w_cam_8 =
	    make_matrix<3, 4>(1.27927685e+03, -8.62928936e+02, -4.43655757e+02, -1.61643315e+04, -5.70077924e+01, -6.79243064e+01, -1.46178659e+03, -8.06921915e+02, 7.90127000e-01, 3.42818000e-01, -5.08109000e-01, 3.67868000e+00);
	Eigen::Matrix<double, 3, 4> const projection_matrix_s110_lidar_ouster_south_into_s110_e_cam_8 =  // this is wrong in https://github.com/tum-traffic-dataset/tum-traffic-dataset-dev-kit/blob/main/src/utils/vis_utils.py
	    make_matrix<3, 4>(-2666.70160799, -655.44528859, -790.96345758, -33010.77350141, 430.89231274, 66.06703744, -2053.70223986, 6630.65222157, -0.00932524, -0.96164431, -0.27414094, 11.41820108);
	Eigen::Matrix<double, 3, 4> const projection_matrix_s110_lidar_ouster_south_into_s110_s_cam_8 =
	    make_matrix<3, 4>(1.54663215e+03, -4.36924071e+02, -2.95583627e+02, 1.31979272e+03, 9.32080566e+01, 4.79035159e+01, -1.48213403e+03, 6.87847813e+02, 7.33260624e-01, 5.97089036e-01, -3.25288544e-01, -1.30114325e+00);

	cv::Mat_<double> const intrinsic_matrix_s110_n_cam_8({3, 3}, {
#include "config/intrinsic_matrix_cam_8"
	                                                             });
	cv::Mat_<double> const intrinsic_matrix_s110_o_cam_8({3, 3}, {
#include "config/intrinsic_matrix_cam_8"
	                                                             });
	cv::Mat_<double> const intrinsic_matrix_s110_s_cam_8({3, 3}, {
#include "config/intrinsic_matrix_cam_8"
	                                                             });
	cv::Mat_<double> const intrinsic_matrix_s110_w_cam_8({3, 3}, {
#include "config/intrinsic_matrix_cam_8"
	                                                             });
	cv::Mat_<double> const distortion_values_s110_n_cam_8{
#include "config/distortion_coefficients_cam_8"
	};
	cv::Mat_<double> const distortion_values_s110_o_cam_8{
#include "config/distortion_coefficients_cam_8"
	};
	cv::Mat_<double> const distortion_values_s110_s_cam_8{
#include "config/distortion_coefficients_cam_8"
	};
	cv::Mat_<double> const distortion_values_s110_w_cam_8{
#include "config/distortion_coefficients_cam_8"
	};

	int const width_s110_n_cam_8 =
#include "config/width_s110_n_cam_8"
	    ;
	int const width_s110_o_cam_8 =
#include "config/width_s110_o_cam_8"
	    ;
	int const width_s110_s_cam_8 =
#include "config/width_s110_s_cam_8"
	    ;
	int const width_s110_w_cam_8 =
#include "config/width_s110_w_cam_8"
	    ;
	int const height_s110_n_cam_8 =
#include "config/height_s110_n_cam_8"
	    ;
	int const height_s110_o_cam_8 =
#include "config/height_s110_o_cam_8"
	    ;
	int const height_s110_s_cam_8 =
#include "config/height_s110_s_cam_8"
	    ;
	int const height_s110_w_cam_8 =
#include "config/height_s110_w_cam_8"
	    ;

	cv::Mat const optimal_camera_matrix_s110_n_cam_8 = cv::getOptimalNewCameraMatrix(intrinsic_matrix_s110_n_cam_8, distortion_values_s110_n_cam_8, {width_s110_n_cam_8, height_s110_n_cam_8}, 0., {width_s110_n_cam_8, height_s110_n_cam_8});
	cv::Mat const optimal_camera_matrix_s110_o_cam_8 = cv::getOptimalNewCameraMatrix(intrinsic_matrix_s110_o_cam_8, distortion_values_s110_o_cam_8, {width_s110_o_cam_8, height_s110_o_cam_8}, 0., {width_s110_o_cam_8, height_s110_o_cam_8});
	cv::Mat const optimal_camera_matrix_s110_s_cam_8 = cv::getOptimalNewCameraMatrix(intrinsic_matrix_s110_s_cam_8, distortion_values_s110_s_cam_8, {width_s110_s_cam_8, height_s110_s_cam_8}, 0., {width_s110_s_cam_8, height_s110_s_cam_8});
	cv::Mat const optimal_camera_matrix_s110_w_cam_8 = cv::getOptimalNewCameraMatrix(intrinsic_matrix_s110_w_cam_8, distortion_values_s110_w_cam_8, {width_s110_w_cam_8, height_s110_w_cam_8}, 0., {width_s110_w_cam_8, height_s110_w_cam_8});

	auto undistortion_maps = [](cv::Mat const& intrinsic_matrix, cv::Mat const& distortion_values, cv::Mat const& optimal_camera_matrix, int width, int height) {
		cv::Mat undistortion_map1, undistortion_map2;
		cv::initUndistortRectifyMap(intrinsic_matrix, distortion_values, cv::Mat_<double>::eye(3, 3), optimal_camera_matrix, {width, height}, CV_32F, undistortion_map1, undistortion_map2);
		return std::tuple{undistortion_map1, undistortion_map2};
	};

	auto const [undistortion_map1_s110_n_cam_8, undistortion_map2_s110_n_cam_8] = undistortion_maps(intrinsic_matrix_s110_n_cam_8, distortion_values_s110_n_cam_8, optimal_camera_matrix_s110_n_cam_8, width_s110_n_cam_8, height_s110_n_cam_8);
	auto const [undistortion_map1_s110_o_cam_8, undistortion_map2_s110_o_cam_8] = undistortion_maps(intrinsic_matrix_s110_o_cam_8, distortion_values_s110_o_cam_8, optimal_camera_matrix_s110_o_cam_8, width_s110_o_cam_8, height_s110_o_cam_8);
	auto const [undistortion_map1_s110_s_cam_8, undistortion_map2_s110_s_cam_8] = undistortion_maps(intrinsic_matrix_s110_s_cam_8, distortion_values_s110_s_cam_8, optimal_camera_matrix_s110_s_cam_8, width_s110_s_cam_8, height_s110_s_cam_8);
	auto const [undistortion_map1_s110_w_cam_8, undistortion_map2_s110_w_cam_8] = undistortion_maps(intrinsic_matrix_s110_w_cam_8, distortion_values_s110_w_cam_8, optimal_camera_matrix_s110_w_cam_8, width_s110_w_cam_8, height_s110_w_cam_8);

}  // namespace config