#include <array>
#include <filesystem>
#include <fstream>

#include "Config.h"
#include "DrawingUtils.h"
#include "EigenUtils.h"
#include "common_output.h"
#include "nlohmann/json.hpp"

int main() {
	// std::ifstream file(std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "tumtraf_v2x_cooperative_perception_dataset" / "val" / "labels_point_clouds" / "s110_lidar_ouster_south_and_vehicle_lidar_robosense_registered" /
	//                    "1688625741_046595374_s110_lidar_ouster_south_and_vehicle_lidar_robosense_registered.json");
	// auto data = nlohmann::json::parse(file);

	Eigen::Matrix<double, 4, 4> lidar_south_to_s110_base = make_matrix<4, 4>(0.21479485, -0.9761028, 0.03296187, -15.87257873,  //
	    0.97627128, 0.21553835, 0.02091894, 2.30019086,                                                                         //
	    -0.02752358, 0.02768645, 0.99923767, 7.48077521,                                                                        //
	    0.00000000, 0.00000000, 0.00000000, 1.00000000);

	Config const config = make_config();
	auto const [map, utm_to_image] = draw_map(std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "visualization" / "2021-07-07_1490_Providentia_Plus_Plus_1_6.xodr", config);

	// data["openlabel"]["frames"]["frame_properties"]["transforms"]["vehicle_lidar_robosense_to_s110_lidar_ouster_south"]

	std::vector<std::filesystem::path> gt_files(std::filesystem::directory_iterator(std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "tumtraf_v2x_cooperative_perception_dataset" / "train" / "labels_point_clouds" /
	                                                                                "s110_lidar_ouster_south_and_vehicle_lidar_robosense_registered"),
	    {});
	std::sort(gt_files.begin(), gt_files.end());
	for (auto const& filepath : gt_files) {
		auto map_clone = map.clone();
		std::ifstream file(filepath);
		auto data = nlohmann::json::parse(file);

		for (auto const& frame : data["openlabel"]["frames"])
			for (auto const& object : frame["objects"]) {
				std::array<double, 10> detection = object["object_data"]["cuboid"]["val"].get<std::array<double, 10>>();
				Eigen::Matrix<double, 4, 1> pos = make_matrix<4, 1>(detection[0], detection[1], detection[2], 1.);
				Eigen::Matrix<double, 4, 1> pos_image = utm_to_image * config.affine_transformation_map_origin_to_utm() * config.affine_transformation_bases_to_map_origin("s110_base") * lidar_south_to_s110_base * pos;
				cv::circle(map_clone, cv::Point(static_cast<int>(pos_image(0)), static_cast<int>(pos_image(1))), 1, cv::Scalar_(255, 255, 255), 5);
			}

		cv::imshow("test", map_clone);
		cv::waitKey(100);
	}
}