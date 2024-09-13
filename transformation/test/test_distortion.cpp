#include <filesystem>
#include <fstream>
#include <opencv2/opencv.hpp>

int main() {
	cv::Mat camera_matrix = cv::Mat_<double>(3, 3);

	{
		std::ifstream lens_intrinsic(std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "TUMTraf_Event_Dataset" / "calibration" / "intrinsic" / "rgb_8mm_matrix.txt");
		// std::ifstream lens_intrinsic(std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "TUMTraf_Event_Dataset" / "calibration" / "intrinsic" / "rgb_8mm_matrix_new.txt");

		for (auto i = 0; i < camera_matrix.rows; ++i) {
			for (auto j = 0; j < camera_matrix.cols; ++j) {
				lens_intrinsic >> camera_matrix.at<double>(i, j);
			}
		}
	}

	cv::Mat distortion_values;

	{
		std::ifstream lens_distortion(std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "TUMTraf_Event_Dataset" / "calibration" / "intrinsic" / "rgb_8mm_dist.txt");

		for (double x = 0.0; lens_distortion >> x; distortion_values.push_back(x))
			;
	}

	cv::Mat image_undistorted_calc;
	cv::Mat image_distorted_calc;
	// cv::Mat image_distorted_initial = cv::imread(std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "camera" / "s110_n_cam_8" / "s110_n_cam_8_images_distorted" / "1690366190031.jpg");
	cv::Mat image_distorted_initial = cv::imread(std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "TUMTraf_Event_Dataset" / "test" / "day" / "images" / "rgb" / "20231218-091930.540629.jpg");
	// cv::Mat image_undistorted_initial = cv::imread(std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "camera" / "s110_n_cam_8" / "s110_n_cam_8_images" / "1690366190031.jpg");
	cv::Mat image_undistorted_initial = cv::imread(std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "TUMTraf_Event_Dataset" / "test" / "day" / "images" / "rgb" / "20231218-091930.540629.jpg");


	cv::resize(image_distorted_initial, image_distorted_initial, cv::Size(1920, 1200), cv::INTER_LINEAR);
	// cv::resize(image_distorted_initial, image_distorted_initial, cv::Size(image_distorted_initial.cols * 3, image_distorted_initial.rows * 3), cv::INTER_LINEAR);
	cv::resize(image_undistorted_initial, image_undistorted_initial, cv::Size(1920, 1200), cv::INTER_LINEAR);
	// cv::resize(image_undistorted_initial, image_undistorted_initial, cv::Size(image_undistorted_initial.cols * 3, image_undistorted_initial.rows * 3), cv::INTER_LINEAR);

	{
		cv::Mat const& image_distorted = image_distorted_initial;

		cv::Mat new_camera_matrix = cv::getOptimalNewCameraMatrix(camera_matrix, distortion_values, cv::Size(image_distorted.cols, image_distorted.rows), 0., cv::Size(image_distorted.cols, image_distorted.rows));
		// for(auto i = 0; i < new_camera_matrix.rows; ++i) {
		//	for(auto j = 0; j < new_camera_matrix.cols; ++j) {
		//		std::cout << new_camera_matrix.at<double>(i, j) << std::endl;
		//	}
		// }

		cv::Mat map_x, map_y;
		cv::initUndistortRectifyMap(camera_matrix, distortion_values, cv::Mat(), new_camera_matrix, cv::Size(image_distorted.cols, image_distorted.rows), CV_32FC1, map_x, map_y);

		cv::Mat image_undistorted;
		cv::remap(image_distorted, image_undistorted, map_x, map_y, cv::INTER_LINEAR, cv::BORDER_TRANSPARENT);

		image_undistorted_calc = image_undistorted;

		// while (true) {
		//	cv::imshow("undistorted/distorted", image_undistorted);
		//	cv::waitKey(0);
		//	cv::imshow("undistorted/distorted", image_distorted);
		//	cv::waitKey(0);
		// }
	}

	{
		cv::Mat const& image_undistorted = image_undistorted_initial;

		cv::Mat new_camera_matrix = cv::getOptimalNewCameraMatrix(camera_matrix, distortion_values, cv::Size(image_undistorted.cols, image_undistorted.rows), 0., cv::Size(image_undistorted.cols, image_undistorted.rows));

		cv::Mat map_x, map_y;
		cv::initInverseRectificationMap(camera_matrix, distortion_values, cv::Mat(), new_camera_matrix, cv::Size(image_undistorted.cols, image_undistorted.rows), CV_32FC1, map_x, map_y);

		cv::Mat image_distorted;
		cv::remap(image_undistorted, image_distorted, map_x, map_y, cv::INTER_LINEAR, cv::BORDER_TRANSPARENT);

		image_distorted_calc = image_distorted;

		while (true) {
			//cv::imshow("undistorted/distorted", image_undistorted);
			//cv::waitKey(0);
			cv::imshow("undistorted/distorted", image_distorted);
			cv::waitKey(0);
			cv::imshow("undistorted/distorted", other_image_distorted);
			cv::waitKey(0);
		}
	}

	// while (true) {
	//	cv::imshow("undistorted", image_undistorted_calc);
	//	cv::waitKey(0);
	//	cv::imshow("undistorted", image_undistorted_initial);
	//	cv::waitKey(0);
	// }

	while (true) {
		cv::imshow("distorted", image_distorted_calc);
		cv::waitKey(0);
		cv::imshow("distorted", image_distorted_initial);
		cv::waitKey(0);
	}
}