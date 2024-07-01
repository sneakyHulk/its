#include "Config.h"

Config make_config(const std::filesystem::path config_directory, const double map_origin_x, const double map_origin_y, const double map_origin_z) {
	Eigen::Matrix<double, 4, 4> affine_transformation_map_origin_to_utm = make_matrix<4, 4>(1., 0., 0., map_origin_x, 0., 1., 0., map_origin_y, 0., 0., 1., map_origin_z, 0., 0., 0., 1.);
	Eigen::Matrix<double, 4, 4> affine_transformation_utm_to_map_origin = affine_transformation_map_origin_to_utm.inverse();

	std::map<std::string, Eigen::Matrix<double, 4, 4>> affine_transformation_map_origin_to_bases;
	std::map<std::string, Eigen::Matrix<double, 4, 4>> affine_transformation_bases_to_map_origin;

	{
		Eigen::Matrix<double, 4, 4> affine_transformation_map_origin_to_s110_base = make_matrix<4, 4>(0., 1., 0., -854.96568588, -1., 0., 0., -631.98486299, 0., 0., 1., 0., 0., 0., 0., 1.);
		affine_transformation_map_origin_to_bases["s110_base"] = affine_transformation_map_origin_to_s110_base;
		Eigen::Matrix<double, 4, 4> affine_transformation_s110_base_to_map_origin = affine_transformation_map_origin_to_s110_base.inverse();
		affine_transformation_bases_to_map_origin["s110_base"] = affine_transformation_s110_base_to_map_origin;

		Eigen::Matrix<double, 4, 4> affine_transformation_map_origin_to_road_s50_base = make_matrix<4, 4>(-0.28401534, -0.95881973, 0., 1.69503701, 0.95881973, -0.28401534, 0., -27.2150203, 0., 0., 1., 0., 0., 0., 0., 1.);
		affine_transformation_map_origin_to_bases["road_s50_base"] = affine_transformation_map_origin_to_road_s50_base;
		Eigen::Matrix<double, 4, 4> affine_transformation_road_s50_base_to_map_origin = affine_transformation_map_origin_to_road_s50_base.inverse();
		affine_transformation_bases_to_map_origin["road_s50_base"] = affine_transformation_road_s50_base_to_map_origin;

		Eigen::Matrix<double, 4, 4> affine_transformation_map_origin_to_road = make_matrix<4, 4>(-0.28401534, -0.95881973, 0., -7.67, 0.95881973, -0.28401534, 0., -25.89, 0., 0., 1., 0., 0., 0., 0., 1.);
		affine_transformation_map_origin_to_bases["road"] = affine_transformation_map_origin_to_road;
		Eigen::Matrix<double, 4, 4> affine_transformation_road_to_map_origin = affine_transformation_map_origin_to_road.inverse();
		affine_transformation_bases_to_map_origin["road"] = affine_transformation_road_to_map_origin;
	}

	std::map<std::string, LensConfig> lens_configs;

	std::regex const lens_config_regex("intrinsic_camera_parameters_([0-9]+)_mm_lens.txt");
	for (const auto& e : std::filesystem::directory_iterator(config_directory / std::filesystem::path("distortion"))) {
		std::string const filename = e.path().filename().generic_string();  // no temporary string allowed for regex_match

		if (std::smatch lens_name_match; std::regex_match(filename, lens_name_match, lens_config_regex)) {
			std::ifstream f(e.path().string());

			lens_configs.emplace(lens_name_match[1].str(), make_lens_config(f));
		}
	}

	std::map<std::string, CameraConfig> camera_configs;
	std::map<std::string, std::tuple<std::string, int, int>> lens_mappings;

	std::regex const camera_config_regex("projection_([a-zA-Z0-9_]+?([0-9]+))\\.json");
	for (const auto& e : std::filesystem::directory_iterator(config_directory)) {
		std::string const filename = e.path().filename().generic_string();  // no temporary string allowed for regex_match

		if (std::smatch camera_name_match; std::regex_match(filename, camera_name_match, camera_config_regex)) {
			std::ifstream f(e.path().string());
			nlohmann::json const config = nlohmann::json::parse(f);

			camera_configs.emplace(camera_name_match[1].str(), make_camera_config(config, camera_name_match[2].str()));
		}
	}

	std::map<std::tuple<std::string, int, int>, cv::Mat> new_camera_matrix;

	for (auto const& [camera_name, camera_config] : camera_configs) {
		auto const lens_mapping = std::tie(camera_config.lens_name(), camera_config.image_width(), camera_config.image_height());
		if (new_camera_matrix.count(lens_mapping)) continue;
		if (!lens_configs.count(camera_config.lens_name())) continue;

		cv::Size image_size(camera_config.image_width(), camera_config.image_height());
		cv::Mat intrinsic_camera_matrix_optimized = cv::getOptimalNewCameraMatrix(lens_configs.at(camera_config.lens_name()).camera_matrix(), lens_configs.at(camera_config.lens_name()).distortion_values(), image_size, 0., image_size);

		new_camera_matrix.emplace(lens_mapping, intrinsic_camera_matrix_optimized);
	}

	return Config{std::forward<decltype(affine_transformation_map_origin_to_utm)>(affine_transformation_map_origin_to_utm), std::forward<decltype(affine_transformation_utm_to_map_origin)>(affine_transformation_utm_to_map_origin),
	    std::forward<decltype(affine_transformation_map_origin_to_bases)>(affine_transformation_map_origin_to_bases), std::forward<decltype(affine_transformation_bases_to_map_origin)>(affine_transformation_bases_to_map_origin),
	    std::forward<decltype(camera_configs)>(camera_configs), std::forward<decltype(lens_configs)>(lens_configs), std::forward<decltype(new_camera_matrix)>(new_camera_matrix)};
}
