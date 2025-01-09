#include <Eigen/Eigen>
#include <filesystem>
#include <fstream>
#include <map>
#include <nlohmann/json.hpp>
#include <opencv2/opencv.hpp>
#include <ranges>
#include <regex>

#include "DrawingUtils.h"
#include "EigenJsonUtils.h"
#include "EigenUtils.h"
#include "common_exception.h"
#include "common_literals.h"
#include "common_output.h"
#include "config.h"

union Cuboid {
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
	Cuboid(std::array<double, 10>&& data) : data(std::forward<decltype(data)>(data)) {}
};

int main() {
	auto const [map, utm_to_image] = draw_map(std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "visualization" / "2021-07-07_1490_Providentia_Plus_Plus_1_6.xodr", 1200, 1920, 5., config::affine_transformation_utm_to_s110_base_north,
	    {{"s110_n_cam_8", {config::projection_matrix_s110_base_north_into_s110_n_cam_8, config::height_s110_n_cam_8, config::width_s110_n_cam_8}},
	        {"s110_o_cam_8", {config::projection_matrix_s110_base_north_into_s110_o_cam_8, config::height_s110_o_cam_8, config::width_s110_o_cam_8}},
	        {"s110_s_cam_8", {config::projection_matrix_s110_base_north_into_s110_s_cam_8, config::height_s110_s_cam_8, config::width_s110_s_cam_8}},
	        {"s110_w_cam_8", {config::projection_matrix_s110_base_north_into_s110_w_cam_8, config::height_s110_w_cam_8, config::width_s110_w_cam_8}}});

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
			Eigen::Vector3d const position = make_matrix<3, 1>(cuboid.pos_x, cuboid.pos_y, cuboid.pos_z);

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

	auto camera_view = [](cv::Mat image, Eigen::Matrix<double, 3, 4> const& projection_matrix, Eigen::Matrix<double, 4, 4> const& affine_transformation_to_camera_projection, std::ranges::viewable_range auto cuboids_types) {
		for (auto const& [cuboid, type] : cuboids_types) {
			Eigen::Vector4d const center_position = make_matrix<4, 1>(cuboid.pos_x, cuboid.pos_y, cuboid.pos_z, 1.);
			Eigen::Vector3d const projected = projection_matrix * affine_transformation_to_camera_projection * center_position;
			if (double x = projected(0) / std::abs(projected(2)), y = projected(1) / std::abs(projected(2)); x < 0 || x > image.cols || y < 0 || y > image.rows) continue;

			Eigen::Quaternion<double> quaternion(cuboid.quat_w, cuboid.quat_x, cuboid.quat_y, cuboid.quat_z);
			Eigen::Vector3d const position = make_matrix<3, 1>(cuboid.pos_x, cuboid.pos_y, cuboid.pos_z);

			Eigen::Vector4d position1 = affine_transformation_to_camera_projection * make_matrix<4, 1>(position + quaternion.matrix() * make_matrix<3, 1>(cuboid.size_x * 0.5, cuboid.size_y * 0.5, cuboid.size_z * 0.5), 1.);
			Eigen::Vector4d position2 = affine_transformation_to_camera_projection * make_matrix<4, 1>(position + quaternion.matrix() * make_matrix<3, 1>(-cuboid.size_x * 0.5, cuboid.size_y * 0.5, cuboid.size_z * 0.5), 1.);
			Eigen::Vector4d position3 = affine_transformation_to_camera_projection * make_matrix<4, 1>(position + quaternion.matrix() * make_matrix<3, 1>(-cuboid.size_x * 0.5, -cuboid.size_y * 0.5, cuboid.size_z * 0.5), 1.);
			Eigen::Vector4d position4 = affine_transformation_to_camera_projection * make_matrix<4, 1>(position + quaternion.matrix() * make_matrix<3, 1>(cuboid.size_x * 0.5, -cuboid.size_y * 0.5, cuboid.size_z * 0.5), 1.);
			Eigen::Vector4d position5 = affine_transformation_to_camera_projection * make_matrix<4, 1>(position + quaternion.matrix() * make_matrix<3, 1>(cuboid.size_x * 0.5, cuboid.size_y * 0.5, -cuboid.size_z * 0.5), 1.);
			Eigen::Vector4d position6 = affine_transformation_to_camera_projection * make_matrix<4, 1>(position + quaternion.matrix() * make_matrix<3, 1>(-cuboid.size_x * 0.5, cuboid.size_y * 0.5, -cuboid.size_z * 0.5), 1.);
			Eigen::Vector4d position7 = affine_transformation_to_camera_projection * make_matrix<4, 1>(position + quaternion.matrix() * make_matrix<3, 1>(-cuboid.size_x * 0.5, -cuboid.size_y * 0.5, -cuboid.size_z * 0.5), 1.);
			Eigen::Vector4d position8 = affine_transformation_to_camera_projection * make_matrix<4, 1>(position + quaternion.matrix() * make_matrix<3, 1>(cuboid.size_x * 0.5, -cuboid.size_y * 0.5, -cuboid.size_z * 0.5), 1.);

			Eigen::Vector3d const projected1 = projection_matrix * position1;
			Eigen::Vector3d const projected2 = projection_matrix * position2;
			Eigen::Vector3d const projected3 = projection_matrix * position3;
			Eigen::Vector3d const projected4 = projection_matrix * position4;
			Eigen::Vector3d const projected5 = projection_matrix * position5;
			Eigen::Vector3d const projected6 = projection_matrix * position6;
			Eigen::Vector3d const projected7 = projection_matrix * position7;
			Eigen::Vector3d const projected8 = projection_matrix * position8;

			std::array<cv::Point, 4> points1;
			points1[0] = cv::Point(static_cast<int>(projected1(0) / std::abs(projected1(2))), static_cast<int>(projected1(1) / std::abs(projected1(2))));
			points1[1] = cv::Point(static_cast<int>(projected2(0) / std::abs(projected2(2))), static_cast<int>(projected2(1) / std::abs(projected2(2))));
			points1[2] = cv::Point(static_cast<int>(projected3(0) / std::abs(projected3(2))), static_cast<int>(projected3(1) / std::abs(projected3(2))));
			points1[3] = cv::Point(static_cast<int>(projected4(0) / std::abs(projected4(2))), static_cast<int>(projected4(1) / std::abs(projected4(2))));

			std::array<cv::Point, 4> points2;
			points2[0] = cv::Point(static_cast<int>(projected5(0) / std::abs(projected5(2))), static_cast<int>(projected5(1) / std::abs(projected5(2))));
			points2[1] = cv::Point(static_cast<int>(projected6(0) / std::abs(projected6(2))), static_cast<int>(projected6(1) / std::abs(projected6(2))));
			points2[2] = cv::Point(static_cast<int>(projected7(0) / std::abs(projected7(2))), static_cast<int>(projected7(1) / std::abs(projected7(2))));
			points2[3] = cv::Point(static_cast<int>(projected8(0) / std::abs(projected8(2))), static_cast<int>(projected8(1) / std::abs(projected8(2))));

			cv::Scalar_<double> color;
			switch (type) {
				case 0_u8: color = cv::Scalar(0, 0, 255); break;
				case 1_u8: color = cv::Scalar(0, 0, 150); break;
				case 2_u8: color = cv::Scalar(255, 0, 0); break;
				case 3_u8: color = cv::Scalar(150, 0, 0); break;
				case 5_u8: color = cv::Scalar(0, 125, 0); break;
				case 7_u8: color = cv::Scalar(0, 255, 0); break;
				case 81_u8: color = cv::Scalar(0, 225, 0); break;
				default: throw;
			}

			cv::polylines(image, points1, true, color, 2);
			cv::polylines(image, points2, true, color, 2);
			cv::line(image, points1[0], points2[0], color, 2);
			cv::line(image, points1[1], points2[1], color, 2);
			cv::line(image, points1[2], points2[2], color, 2);
			cv::line(image, points1[3], points2[3], color, 2);

			cv::line(image, points1[0], points1[2], color, 1);
			cv::line(image, points1[1], points1[3], color, 1);
			cv::line(image, points2[0], points2[2], color, 1);
			cv::line(image, points2[1], points2[3], color, 1);

			cv::line(image, points1[0], points2[1], color, 1);
			cv::line(image, points1[1], points2[2], color, 1);
			cv::line(image, points1[2], points2[3], color, 1);
			cv::line(image, points1[3], points2[0], color, 1);
			cv::line(image, points1[0], points2[3], color, 1);
			cv::line(image, points1[1], points2[0], color, 1);
			cv::line(image, points1[2], points2[1], color, 1);
			cv::line(image, points1[3], points2[2], color, 1);
		}

		return image;
	};

	auto files = std::ranges::to<std::vector<std::filesystem::directory_entry>>(std::filesystem::directory_iterator(
	    std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "tumtraf_v2x_cooperative_perception_dataset" / "train" / "labels_point_clouds" / "s110_lidar_ouster_south_and_vehicle_lidar_robosense_registered"));
	std::ranges::sort(files);

	cv::VideoWriter bird_eye_view_video(std::filesystem::path(CMAKE_SOURCE_DIR) / "result" / "bird_eye_view_video.mp4", cv::VideoWriter::fourcc('m', 'p', '4', 'v'), 10., {1920, 1200});
	cv::VideoWriter vehicle_view_video(std::filesystem::path(CMAKE_SOURCE_DIR) / "result" / "vehicle_view_video.mp4", cv::VideoWriter::fourcc('m', 'p', '4', 'v'), 10., {1920, 1200});
	cv::VideoWriter infrastructure_north_view_video(std::filesystem::path(CMAKE_SOURCE_DIR) / "result" / "infrastructure_north_view_video.mp4", cv::VideoWriter::fourcc('m', 'p', '4', 'v'), 10., {1920, 1200});
	cv::VideoWriter infrastructure_south_view_video(std::filesystem::path(CMAKE_SOURCE_DIR) / "result" / "infrastructure_south_view_video.mp4", cv::VideoWriter::fourcc('m', 'p', '4', 'v'), 10., {1920, 1200});

	cv::VideoWriter combination_view_video(std::filesystem::path(CMAKE_SOURCE_DIR) / "result" / "combination_view_video.mp4", cv::VideoWriter::fourcc('m', 'p', '4', 'v'), 10., {1920 * 2, 1200 * 2});

	cv::Mat bird_eye_view_image, vehicle_view_image, infrastructure_north_view_image, infrastructure_south_view_image, combination_view_image;

	auto old_timestamp = 0.;
	for (auto const& file : files) {
		std::ifstream labeled_data_file(file.path());
		nlohmann::json labeled_data = nlohmann::json::parse(labeled_data_file);

		auto timestamp = labeled_data["openlabel"]["frames"].front()["frame_properties"]["timestamp"].get<double>();
		common::println("frame_time: ", timestamp - old_timestamp);
		old_timestamp = timestamp;

		nlohmann::json objects = labeled_data["openlabel"]["frames"].front()["objects"];
		auto cuboids = objects | std::ranges::views::transform([](auto const& object) { return Cuboid{object["object_data"]["cuboid"]["val"].template get<std::array<double, 10>>()}; });
		auto positions = cuboids | std::ranges::views::transform([](auto const& cuboid) { return cuboid.pos; });
		auto types = objects | std::ranges::views::transform([](auto const& object) {
			if (auto type = object["object_data"]["type"].template get<std::string>(); type == "PEDESTRIAN") {
				return 0_u8;
			} else if (type == "BICYCLE") {
				return 1_u8;
			} else if (type == "CAR" || type == "EMERGENCY_VEHICLE" || type == "VAN") {
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

		bird_eye_view_image = bird_eye_view(std::ranges::views::zip(cuboids, types));

		auto filenames = labeled_data["openlabel"]["frames"].front()["frame_properties"]["image_file_names"] | std::ranges::views::transform([](auto const& data) { return data.template get<std::string>(); });
		for (std::string filename : filenames) {
			static std::regex const filename_filter("([0-9]+)_([0-9]+)_([a-z0-9_]+)\\.[a-z]+");
			std::smatch matches;
			std::regex_match(filename, matches, filename_filter);

			cv::Mat image = cv::imread(std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "tumtraf_v2x_cooperative_perception_dataset" / "train" / "images" / matches[3].str() / filename);

			if (matches[3].str() == "s110_camera_basler_north_8mm") {
				// cv::remap(image, image, config::undistortion_map1_s110_n_cam_8, config::undistortion_map2_s110_n_cam_8, cv::INTER_LINEAR);
				camera_view(image, config::projection_matrix_s110_lidar_ouster_south_into_s110_n_cam_8, config::affine_transformation_nothing, std::ranges::views::zip(cuboids, types));

				infrastructure_north_view_image = image.clone();
				// cv::imshow("Camera View North", image);
				// cv::waitKey(0);
			}

			if (matches[3].str() == "s110_camera_basler_south2_8mm") {
				camera_view(image, config::projection_matrix_s110_lidar_ouster_south_into_s110_s_cam_8, config::affine_transformation_nothing, std::ranges::views::zip(cuboids, types));

				infrastructure_south_view_image = image.clone();
				// cv::imshow("Camera View South", image);
				// cv::waitKey(0);
			}

			if (matches[3].str() == "vehicle_camera_basler_16mm") {
				Eigen::Matrix<double, 4, 4> affine_transformation_vehicle_lidar_robosense_to_s110_lidar_ouster_south =
				    labeled_data["openlabel"]["frames"].front()["frame_properties"]["transforms"]["vehicle_lidar_robosense_to_s110_lidar_ouster_south"]["transform_src_to_dst"]["matrix4x4"].template get<Eigen::Matrix<double, 4, 4>>();

				camera_view(
				    image, config::projection_matrix_vehicle_lidar_robosense_into_vehicle_camera_basler_16mm, affine_transformation_vehicle_lidar_robosense_to_s110_lidar_ouster_south.inverse(), std::ranges::views::zip(cuboids, types));

				vehicle_view_image = image.clone();
				// cv::imshow("Camera View", image);
				// cv::waitKey(0);
			}
		}

		bird_eye_view_video.write(bird_eye_view_image);
		vehicle_view_video.write(vehicle_view_image);
		infrastructure_north_view_video.write(infrastructure_north_view_image);
		infrastructure_south_view_video.write(infrastructure_south_view_image);

		cv::Mat tmp1;
		cv::Mat tmp2;
		cv::hconcat(bird_eye_view_image, infrastructure_north_view_image, tmp1);
		cv::hconcat(infrastructure_south_view_image, vehicle_view_image, tmp2);
		cv::vconcat(tmp1, tmp2, combination_view_image);

		combination_view_video.write(combination_view_image);
	}
}
