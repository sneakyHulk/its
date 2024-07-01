#include "CameraConfig.h"

CameraConfig make_camera_config(nlohmann::json const& config, std::string lens_name) {
	std::string base_name = config["projections"][0]["extrinsic_transform"]["src"].template get<std::string>();

	int image_width = config["image_width"];
	int image_height = config["image_height"];

	// std::vector<std::vector<double>> const projection_matrix_stl = config["projections"][0]["projection_matrix"].template get<std::vector<std::vector<double>>>();
	Eigen::Matrix<double, 3, 4> projection_matrix = config["projections"][0]["projection_matrix"].template get<Eigen::Matrix<double, 3, 4>>();
	Eigen::Matrix<double, 3, 3> KR = projection_matrix(Eigen::all, Eigen::seq(0, Eigen::last - 1));
	Eigen::Matrix<double, 3, 3> KR_inv = KR.inverse();
	Eigen::Matrix<double, 3, 1> C = projection_matrix(Eigen::all, Eigen::last);
	Eigen::Matrix<double, 3, 1> translation_camera = -KR_inv * C;

	return CameraConfig{std::forward<decltype(base_name)>(base_name), std::forward<decltype(lens_name)>(lens_name), std::forward<decltype(image_width)>(image_width), std::forward<decltype(image_height)>(image_height),
	    std::forward<decltype(projection_matrix)>(projection_matrix), std::forward<decltype(KR)>(KR), std::forward<decltype(KR_inv)>(KR_inv), std::forward<decltype(C)>(C), std::forward<decltype(translation_camera)>(translation_camera)};
}