#include <Eigen/Eigen>
#include <filesystem>
#include <fstream>
#include <generator>
#include <map>
#include <nlohmann/json.hpp>
#include <opencv2/opencv.hpp>
#include <ranges>

#include "DrawingUtils.h"
#include "EigenUtils.h"
#include "common_exception.h"
#include "common_literals.h"
#include "common_output.h"

namespace config {
	Eigen::Matrix<double, 4, 4> const affine_transformation_rotate_90 = make_matrix<4, 4>(0., -1., 0., 0., 1., 0., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1.);

	Eigen::Matrix<double, 4, 4> const affine_transformation_map_origin_to_utm = make_matrix<4, 4>(1., 0., 0., 695942.4856864865, 0., 1., 0., 5346521.128436302, 0., 0., 1., 485.0095881917835, 0., 0., 0., 1.);
	Eigen::Matrix<double, 4, 4> const affine_transformation_map_origin_to_s110_base = make_matrix<4, 4>(0., 1., 0., -854.96568588, -1., 0., 0., -631.98486299, 0., 0., 1., 0., 0., 0., 0., 1.);
	// s110_base is 90° rotated, so to make the s110_base
	Eigen::Matrix<double, 4, 4> const affine_transformation_utm_to_s110_base_north = affine_transformation_rotate_90.inverse() * affine_transformation_map_origin_to_s110_base * affine_transformation_map_origin_to_utm.inverse();

	Eigen::Matrix<double, 4, 4> affine_transformation_s110_base_to_s110_lidar_ouster_south =
	    make_matrix<4, 4>(0.21479485, -0.9761028, 0.03296187, -15.87257873, 0.97627128, 0.21553835, 0.02091894, 2.30019086, -0.02752358, 0.02768645, 0.99923767, 7.48077521, 0.00000000, 0.00000000, 0.00000000, 1.00000000);

	Eigen::Matrix<double, 4, 4> affine_transformation_utm_to_s110_lidar_ouster_south =
	    affine_transformation_s110_base_to_s110_lidar_ouster_south.inverse() * affine_transformation_map_origin_to_s110_base * affine_transformation_map_origin_to_utm.inverse();

	// projection matrices
	Eigen::Matrix<double, 3, 4> projection_matrix_s110_base_into_s110_n_cam_8 = make_matrix<3, 4>(1438.3576577095812, -503.41452115064544, -559.0764073903879, 3237.0045449442528, -228.58158332112308, -181.86601178450658, -1383.615751349628,
	    12787.267956267566, 0.6294751019871989, 0.5227093436048773, -0.5749226366097999, 4.171119732791681);
	Eigen::Matrix<double, 3, 4> projection_matrix_s110_base_north_into_s110_n_cam_8 = projection_matrix_s110_base_into_s110_n_cam_8 * affine_transformation_rotate_90;

	Eigen::Matrix<double, 3, 4> projection_matrix_s110_base_into_s110_o_cam_8 = make_matrix<3, 4>(-1132.628549283461, -1130.380492069529, -449.9248107105663, -12810.311863988321, -22.56375289356649, 122.73752058700626, -1623.036471413328,
	    12182.17200003528, 0.18755945751474123, -0.8689882111124836, -0.45790931290409675, 7.907850338588673);
	Eigen::Matrix<double, 3, 4> projection_matrix_s110_base_north_into_s110_o_cam_8 = projection_matrix_s110_base_into_s110_o_cam_8 * affine_transformation_rotate_90;

	Eigen::Matrix<double, 3, 4> projection_matrix_s110_base_into_s110_s_cam_8 = make_matrix<3, 4>(751.1987458277946, 1410.9726419527894, -359.06755450265314, 12525.83827191849, -59.06474175441968, 65.32364938123933, -1416.6989398379606,
	    11388.58258654808, -0.4309406392705249, 0.8334612519798386, -0.34587932414833933, -7.259570302320384);
	Eigen::Matrix<double, 3, 4> projection_matrix_s110_base_north_into_s110_s_cam_8 = projection_matrix_s110_base_into_s110_s_cam_8 * affine_transformation_rotate_90;

	Eigen::Matrix<double, 3, 4> projection_matrix_s110_base_into_s110_w_cam_8 = make_matrix<3, 4>(1183.4532206493716, 1046.0102689935916, -443.03527355579865, 5016.366482965497, 32.163492565225496, -134.9151551976772, -1441.9234069037752,
	    12790.570485915014, -0.1389221303517314, 0.8697766646509542, -0.47348621450597506, 3.172645642076077);
	Eigen::Matrix<double, 3, 4> projection_matrix_s110_base_north_into_s110_w_cam_8 = projection_matrix_s110_base_into_s110_w_cam_8 * affine_transformation_rotate_90;

	// intrinsic matrices
	Eigen::Matrix<double, 3, 3> intrinsic_s110_n_cam_8 = make_matrix<3, 3>(1400.3096617691212, 0.0, 967.7899705163408, 0.0, 1403.041082755918, 581.7195041357244, 0.0, 0.0, 1.0);
	Eigen::Matrix<double, 3, 3> intrinsic_s110_o_cam_8 = make_matrix<3, 3>(1400.3096617691212, 0.0, 967.7899705163408, 0.0, 1403.041082755918, 581.7195041357244, 0.0, 0.0, 1.0);
	Eigen::Matrix<double, 3, 3> intrinsic_s110_s_cam_8 = make_matrix<3, 3>(1400.3096617691212, 0.0, 967.7899705163408, 0.0, 1403.041082755918, 581.7195041357244, 0.0, 0.0, 1.0);
	Eigen::Matrix<double, 3, 3> intrinsic_s110_w_cam_8 = make_matrix<3, 3>(1400.3096617691212, 0.0, 967.7899705163408, 0.0, 1403.041082755918, 581.7195041357244, 0.0, 0.0, 1.0);

}  // namespace config

struct BirdEyeVis {
	std::array<double, 3> position;
	std::array<double, 2> velocity;
};

// cv::Mat bird_eye_vis(std::vector<std::array<double, 3>> ) {
//
//
//	auto tmp = _map.clone();
//	for (auto const &object : data.objects) {
//		cv::Scalar color;
//		switch (object.object_class) {
//			case 0_u8: color = cv::Scalar(0, 0, 255); break;
//			case 1_u8: color = cv::Scalar(0, 0, 150); break;
//			case 2_u8: color = cv::Scalar(255, 0, 0); break;
//			case 3_u8: color = cv::Scalar(150, 0, 0); break;
//			case 5_u8: color = cv::Scalar(0, 150, 0); break;
//			case 7_u8: color = cv::Scalar(0, 255, 0); break;
//			default: throw;
//		}
//
//		Eigen::Vector4d const position = _utm_to_image * make_matrix<4, 1>(object.position[0], object.position[1], 0., 1.);  // * config.affine_transformation_map_origin_to_utm()
//
//		cv::circle(tmp, cv::Point(static_cast<int>(position(0)), static_cast<int>(position(1))), 1, color, 10);
//	}
//
//
//
// }

union cuboid {
	struct {
		double pos_x;
		double pos_y;
		double pos_z;
		double quat_x;
		double quat_y;
		double quat_z;
		double quat_w;
		double size_x;
		double size_y;
		double size_z;
	};
	struct {
		std::array<double, 3> pos;
		std::array<double, 4> quat_last_order;
		std::array<double, 3> size;
	};
	std::array<double, 10> data;
	cuboid(std::array<double, 10>&& data) : data(std::forward<decltype(data)>(data)) {}
};

int main() {
	auto const [map, utm_to_image] = draw_map(std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "visualization" / "2021-07-07_1490_Providentia_Plus_Plus_1_6.xodr", 1920, 1200, 4., config::affine_transformation_utm_to_s110_base_north,
	    {{"s110_n_cam_8", {config::projection_matrix_s110_base_north_into_s110_n_cam_8, 1920, 1200}}, {"s110_o_cam_8", {config::projection_matrix_s110_base_north_into_s110_o_cam_8, 1920, 1200}},
	        {"s110_s_cam_8", {config::projection_matrix_s110_base_north_into_s110_s_cam_8, 1920, 1200}}, {"s110_w_cam_8", {config::projection_matrix_s110_base_north_into_s110_w_cam_8, 1920, 1200}}});

	auto bird_eye_view_old = [&map, coords_to_image = utm_to_image * config::affine_transformation_utm_to_s110_lidar_ouster_south.inverse()](std::ranges::viewable_range auto positions_types) -> cv::Mat {
		auto tmp = map.clone();
		for (auto const& [position, type] : positions_types) {
			Eigen::Vector4d const position_eigen = coords_to_image * make_matrix<4, 1>(position[0], position[1], position[2], 1.);

			switch (type) {
				case 0_u8: cv::circle(tmp, cv::Point(static_cast<int>(position_eigen(0)), static_cast<int>(position_eigen(1))), 1, cv::Scalar(0, 0, 255), 3); break;
				case 1_u8: cv::circle(tmp, cv::Point(static_cast<int>(position_eigen(0)), static_cast<int>(position_eigen(1))), 1, cv::Scalar(0, 0, 150), 5); break;
				case 2_u8: cv::circle(tmp, cv::Point(static_cast<int>(position_eigen(0)), static_cast<int>(position_eigen(1))), 1, cv::Scalar(255, 0, 0), 8); break;
				case 3_u8: cv::circle(tmp, cv::Point(static_cast<int>(position_eigen(0)), static_cast<int>(position_eigen(1))), 1, cv::Scalar(150, 0, 0), 6); break;
				case 5_u8: cv::circle(tmp, cv::Point(static_cast<int>(position_eigen(0)), static_cast<int>(position_eigen(1))), 1, cv::Scalar(0, 150, 0), 15); break;
				case 7_u8: cv::circle(tmp, cv::Point(static_cast<int>(position_eigen(0)), static_cast<int>(position_eigen(1))), 1, cv::Scalar(0, 255, 0), 15); break;
				case 81_u8: cv::circle(tmp, cv::Point(static_cast<int>(position_eigen(0)), static_cast<int>(position_eigen(1))), 1, cv::Scalar(0, 255, 0), 10); break;
				default: throw;
			}
		}

		return tmp;
	};

	auto bird_eye_view = [&map, coords_to_image = utm_to_image * config::affine_transformation_utm_to_s110_lidar_ouster_south.inverse()](std::ranges::viewable_range auto cuboids_types) -> cv::Mat {
		auto tmp = map.clone();
		for (auto const& [cuboid, type] : cuboids_types) {
			Eigen::Quaternion<double> quaternion(cuboid.quat_w, cuboid.quat_x, cuboid.quat_y, cuboid.quat_z);
			Eigen::Vector3d const position = make_matrix<3, 1>(cuboid.pos_x, cuboid.pos_y, cuboid.pos_y);

			Eigen::Vector4d position1 = make_matrix<4, 1>(position + quaternion.matrix() * make_matrix<3, 1>(cuboid.size_x * 0.5, cuboid.size_y * 0.5, cuboid.size_z * 0.5), 1.);
			Eigen::Vector4d position2 = make_matrix<4, 1>(position + quaternion.matrix() * make_matrix<3, 1>(-cuboid.size_x * 0.5, cuboid.size_y * 0.5, cuboid.size_z * 0.5), 1.);
			Eigen::Vector4d position3 = make_matrix<4, 1>(position + quaternion.matrix() * make_matrix<3, 1>(-cuboid.size_x * 0.5, -cuboid.size_y * 0.5, cuboid.size_z * 0.5), 1.);
			Eigen::Vector4d position4 = make_matrix<4, 1>(position + quaternion.matrix() * make_matrix<3, 1>(cuboid.size_x * 0.5, -cuboid.size_y * 0.5, cuboid.size_z * 0.5), 1.);

			position1 = coords_to_image * position1;
			position2 = coords_to_image * position2;
			position3 = coords_to_image * position3;
			position4 = coords_to_image * position4;

			std::array<cv::Point, 4> points;
			points[0] = cv::Point(static_cast<int>(position1(0)), static_cast<int>(position1(1)));
			points[1] = cv::Point(static_cast<int>(position2(0)), static_cast<int>(position2(1)));
			points[2] = cv::Point(static_cast<int>(position3(0)), static_cast<int>(position3(1)));
			points[3] = cv::Point(static_cast<int>(position4(0)), static_cast<int>(position4(1)));

			switch (type) {
				case 0_u8: cv::fillConvexPoly(tmp, points, cv::Scalar(0, 0, 255)); break;
				case 1_u8: cv::fillConvexPoly(tmp, points, cv::Scalar(0, 0, 150)); break;
				case 2_u8: cv::fillConvexPoly(tmp, points, cv::Scalar(255, 0, 0)); break;
				case 3_u8: cv::fillConvexPoly(tmp, points, cv::Scalar(150, 0, 0)); break;
				case 5_u8: cv::fillConvexPoly(tmp, points, cv::Scalar(0, 125, 0)); break;
				case 7_u8: cv::fillConvexPoly(tmp, points, cv::Scalar(0, 255, 0)); break;
				case 81_u8: cv::fillConvexPoly(tmp, points, cv::Scalar(0, 225, 0)); break;
				default: throw;
			}
		}

		return tmp;
	};

	auto files = std::ranges::to<std::vector<std::filesystem::directory_entry>>(std::filesystem::directory_iterator(
	    std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "tumtraf_v2x_cooperative_perception_dataset" / "train" / "labels_point_clouds" / "s110_lidar_ouster_south_and_vehicle_lidar_robosense_registered"));
	std::ranges::sort(files);

	for (auto const& file : files) {
		std::ifstream labeled_data_file(file.path());
		nlohmann::json labeled_data = nlohmann::json::parse(labeled_data_file);
		nlohmann::json objects = labeled_data["openlabel"]["frames"].front()["objects"];

		auto cuboids = objects | std::ranges::views::transform([](auto const& object) { return cuboid{object["object_data"]["cuboid"]["val"].template get<std::array<double, 10>>()}; });
		auto positions = cuboids | std::ranges::views::transform([](auto const& cuboid) { return cuboid.pos; });
		auto types = objects | std::ranges::views::transform([](auto const& object) {
			if (auto type = object["object_data"]["type"].template get<std::string>(); type == "PEDESTRIAN") {
				return 0_u8;
			} else if (type == "BICYCLE") {
				return 1_u8;
			} else if (type == "CAR" && type == "EMERGENCY_VEHICLE") {
				return 2_u8;
			} else if (type == "VAN") {
				return 2_u8;
			} else if (type == "MOTORCYCLE") {
				return 3_u8;
			} else if (type == "BUS") {
				return 5_u8;
			} else if (type == "TRUCK") {
				return 7_u8;
			} else if (type == "TRAILER") {
				return 81_u8;
			} else {
				throw common::Exception(type, " not implemented!");
			}
		});

		cv::imshow("Bird Eye View", bird_eye_view(std::ranges::views::zip(cuboids, types)));
		cv::waitKey(0);
	}

	// auto cuboid_view2 = [&objects]() -> std::generator<cuboid> {
	//	for (auto const& object : objects) {
	//		co_yield object["object_data"]["cuboid"]["val"].get<std::array<double, 10>>();
	//	}
	// };

	//
	//
	// Eigen::Matrix<double, 3, 4> projection_matrix = config["projections"][0]["projection_matrix"].template get<Eigen::Matrix<double, 3, 4>>();
	// Eigen::Matrix<double, 3, 3> KR = projection_matrix(Eigen::all, Eigen::seq(0, Eigen::last - 1));
	// Eigen::Matrix<double, 3, 3> KR_inv = KR.inverse();
	// Eigen::Matrix<double, 3, 1> C = projection_matrix(Eigen::all, Eigen::last);
	// Eigen::Matrix<double, 3, 1> translation_camera = -KR_inv * C;

	std::cout.precision(15);

	common::println(config::affine_transformation_utm_to_s110_lidar_ouster_south.inverse() * make_matrix<4, 1>(20.106891966109295, 37.67467354759484, -6.107125427890823, 1));

	Eigen::Quaternion<double> test(0.6486401156865809, 0, 0, -0.7610952636313663);

	Eigen::Matrix3d rotationMatrix = test.matrix();
	common::println(rotationMatrix);

	Eigen::Matrix<double, 4, 1> test2;
	test2 << rotationMatrix * make_matrix<3, 1>(1, 0, 0), 0.;
	common::println(test2);

	common::println(config::affine_transformation_utm_to_s110_lidar_ouster_south.inverse() * (test2 + make_matrix<4, 1>(15.442965014747536, 32.05620718755819, -6.310666575431823, 1)));

	cv::imshow("test", map);
	cv::waitKey(0);
	// common::println(get_intrinsic_rotation_matrix(intrinsic_s110_n_cam_8, translation_s110_n_cam_8, rotation_s110_n_cam_8));
	// common::println(get_translation_from_camera_matrix(get_camera_matrix(intrinsic_s110_n_cam_8, translation_s110_n_cam_8, rotation_s110_n_cam_8)));
}
