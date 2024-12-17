#include <../../thirdparty/json/include/nlohmann/json.hpp>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <tuple>
#include <vector>

#include "../../thirdparty/common/include/common_output.h"
#include "../../tracking/include/association_functions.h"
#include "../../transformation/include/Config.h"
#include "yolo_function.h"

int main(int argc, char* argv[]) {
	Config const config = make_config();

	cv::Mat camera_matrix = cv::Mat_<double>(3, 3);
	{
		std::ifstream lens_intrinsic(std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "TUMTraf_Event_Dataset" / "calibration" / "intrinsic" / "rgb_8mm_matrix.txt");
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

	cv::Mat new_camera_matrix = cv::getOptimalNewCameraMatrix(camera_matrix, distortion_values, cv::Size(1920, 1200), 0., cv::Size(1920, 1200));

	cv::Mat map_x, map_y;
	cv::initInverseRectificationMap(camera_matrix, distortion_values, cv::Mat(), new_camera_matrix, cv::Size(1920, 1200), CV_32FC1, map_x, map_y);



	std::regex const timestamp_regex("([0-9][0-9][0-9][0-9])([0-9][0-9])([0-9][0-9])-([0-9][0-9])([0-9][0-9])([0-9][0-9])\\.([0-9]+)");

	for (const auto& image_path : std::filesystem::directory_iterator(std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "TUMTraf_Event_Dataset" / "val" / "images" / "rgb")) {
		cv::Mat orig_image = cv::imread(image_path.path());
		cv::Mat resize_image;
		cv::resize(orig_image, resize_image, cv::Size(1920, 1200), 0, 0, cv::INTER_LANCZOS4);
		ImageData data;
		data.source = "s110_w_cam_8";

		cv::remap(resize_image, data.image, map_x, map_y, cv::INTER_LINEAR, cv::BORDER_TRANSPARENT);
		Detections2D distorted_detections = run_yolo<480, 640>(data);

		for (auto& detection : distorted_detections.objects) {
			auto& [left, top, right, bottom] = detection.bbox;

			std::tie(left, top) = config.undistort_point(data.source, left, top);
			std::tie(right, bottom) = config.undistort_point(data.source, right, bottom);
		}

		data.image = resize_image;
		Detections2D undistorted_detections = run_yolo<480, 640>(data);

		Detections2D ground_truth;

		{
			std::ifstream gt_data(std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "TUMTraf_Event_Dataset" / "val" / "yolo_labels_rgb" / (image_path.path().stem().string() + ".txt"));

			// nlohmann::json gt_json = nlohmann::json::parse(gt_data);
			//  for (auto const& e : gt_json["openlabel"]["frames"][0]["objects"]) {
			//	auto center_x = e["object_data"]["bbox"]["val"][0];
			//	auto center_y = e["object_data"]["bbox"]["val"][1];
			//	auto center_width = e["object_data"]["bbox"]["val"][2];
			//	auto center_height = e["object_data"]["bbox"]["val"][3];
			//  }

			while (!gt_data.eof()) {
				Detection2D current;
				double center_x;
				double center_y;
				double width;
				double height;
				gt_data >> current.object_class >> center_x >> center_y >> width >> height;

				center_x *= 1920.;
				center_y *= 1200.;
				width *= 1920.;
				height *= 1200.;

				current.bbox.left = center_x - width / 2.;
				current.bbox.top = center_y - height / 2.;

				current.bbox.right = center_x + width / 2.;
				current.bbox.bottom = center_y + height / 2.;

				ground_truth.objects.push_back(current);
			}
		}

		for (auto e : undistorted_detections.objects) {
			cv::rectangle(data.image, cv::Point2d(e.bbox.left, e.bbox.top), cv::Point2d(e.bbox.right, e.bbox.bottom), cv::Scalar_<int>(255, 0, 0), 1);
		}

		for (auto e : distorted_detections.objects) {
			cv::rectangle(data.image, cv::Point2d(e.bbox.left, e.bbox.top), cv::Point2d(e.bbox.right, e.bbox.bottom), cv::Scalar_<int>(0, 0, 255), 1);
		}

		for (auto e : ground_truth.objects) {
			cv::rectangle(data.image, cv::Point2d(e.bbox.left, e.bbox.top), cv::Point2d(e.bbox.right, e.bbox.bottom), cv::Scalar_<int>(0, 255, 0), 1);
		}

		cv::imshow("undistorted_vs_gt", data.image);
		cv::waitKey(0);

		// cv::Mat other_image_distorted = cv::imread(std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "camera" / "s110_w_cam_8" / "s110_w_cam_8_images_distorted" / "1690366214422.jpg");
		// cv::Mat other_image = cv::imread(std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "camera" / "s110_w_cam_8" / "s110_w_cam_8_images" / "1690366214422.jpg");
		// cv::imshow("undistorted/distorted", resize_image);
		// cv::waitKey(0);
		// cv::imshow("undistorted/distorted", other_image);
		// cv::waitKey(0);

		auto i = 0;
	}
}

/*
std::smatch timestamp_match;
std::string timestamp_steam = image_path.path().stem().generic_string();
std::regex_match(timestamp_steam, timestamp_match, timestamp_regex);
std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::year(std::stoll(timestamp_match[1]))) + std::chrono::month(std::stoll(timestamp_match[2])) + std::chrono::day(std::stoll(timestamp_match[3])) +
std::chrono::hours(std::stoll(timestamp_match[5])) + std::chrono::minutes(std::stoll(timestamp_match[6])) + std::chrono::seconds(std::stoll(timestamp_match[7])) +

data.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::milliseconds(std::stoll(undistorted_image_files[i].path().stem()))).count();

int main(int argc, char* argv[]) {
    Config const config = make_config();

    for (const auto& folder : std::filesystem::directory_iterator(std::filesystem::path(CMAKE_SOURCE_DIR) / std::filesystem::path("data/camera"))) {
        auto iterator_distorted = std::filesystem::directory_iterator(folder.path() / std::filesystem::path(folder.path().filename().generic_string() + "_images_distorted"));
        auto iterator_undistorted = std::filesystem::directory_iterator(folder.path() / std::filesystem::path(folder.path().filename().generic_string() + "_images"));
        auto iterator_detection = std::filesystem::directory_iterator(folder.path() / std::filesystem::path(folder.path().filename().generic_string() + "_detections"));

        std::vector<std::filesystem::directory_entry> undistorted_image_files(iterator_undistorted, {});
        std::vector<std::filesystem::directory_entry> distorted_image_files(iterator_distorted, {});
        std::vector<std::filesystem::directory_entry> detection_files(iterator_detection, {});

        std::sort(undistorted_image_files.begin(), undistorted_image_files.end(), [](auto const& e1, auto const& e2) { return e1.path().stem() > e2.path().stem(); });
        std::sort(distorted_image_files.begin(), distorted_image_files.end(), [](auto const& e1, auto const& e2) { return e1.path().stem() > e2.path().stem(); });
        std::sort(detection_files.begin(), detection_files.end(), [](auto const& e1, auto const& e2) { return e1.path().stem() > e2.path().stem(); });

        common::println("Dealing with ", undistorted_image_files.size(), " and ", distorted_image_files.size(), " files!");

        for (auto i = 0, detection_index = 0; i < undistorted_image_files.size(); ++i) {
            Detections2D undistorted_detections;
            Detections2D ground_truth;
            {
                ImageData data;
                data.image = cv::imread(undistorted_image_files[i].path(), cv::IMREAD_COLOR);
                data.width = data.image.cols;
                data.height = data.image.rows;
                data.source = undistorted_image_files[i].path().parent_path().parent_path().filename();
                data.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::milliseconds(std::stoll(undistorted_image_files[i].path().stem()))).count();
                undistorted_detections = run_yolo<480, 640>(data);

                for (;; ++detection_index) {
                    if (detection_index < detection_files.size()) {
                        ground_truth.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::milliseconds(std::stoll(detection_files[detection_index].path().stem()))).count();

                        if (ground_truth.timestamp == data.timestamp) {
                            ground_truth.source = detection_files[detection_index].path().parent_path().parent_path().filename();

                            std::ifstream detection_json_stream(detection_files[detection_index].path());
                            nlohmann::json detection_json = nlohmann::json::parse(detection_json_stream);

                            for (auto const& e : detection_json["object_list"]) {
                                Detection2D detection;

                                detection.bbox.top = e["boundingbox"][0].get<double>() * data.height;
                                detection.bbox.bottom = e["boundingbox"][1].get<double>() * data.height;
                                detection.bbox.left = e["boundingbox"][2].get<double>() * data.width;
                                detection.bbox.right = e["boundingbox"][3].get<double>() * data.width;

                                detection.object_class = e["object_class"].get<std::uint8_t>();

                                detection.conf = e["classification_confidence"][detection.object_class];

                                ground_truth.objects.push_back(detection);
                            }

                            break;
                        }
                    } else {
                        throw std::logic_error("No ground truth found!");
                    }
                }
            }

            Detections2D distorted_detections;
            {
                ImageData data;
                data.image = cv::imread(distorted_image_files[i].path(), cv::IMREAD_COLOR);
                data.width = data.image.cols;
                data.height = data.image.rows;
                data.source = distorted_image_files[i].path().parent_path().parent_path().filename();
                data.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::milliseconds(std::stoll(undistorted_image_files[i].path().stem()))).count();
                distorted_detections = run_yolo<480, 640>(data);

                for (auto& detection : distorted_detections.objects) {
                    auto& [left, top, right, bottom] = detection.bbox;

                    std::tie(left, top) = config.undistort_point(data.source, left, top);
                    std::tie(right, bottom) = config.undistort_point(data.source, right, bottom);
                }

                if (distorted_detections.objects.size() != undistorted_detections.objects.size()) {
                    for (auto e : undistorted_detections.objects) {
                        cv::rectangle(data.image, cv::Point2d(e.bbox.left, e.bbox.top), cv::Point2d(e.bbox.right, e.bbox.bottom), cv::Scalar_<int>(0, 0, 255), 1);
                    }

                    for (auto e : distorted_detections.objects) {
                        cv::rectangle(data.image, cv::Point2d(e.bbox.left, e.bbox.top), cv::Point2d(e.bbox.right, e.bbox.bottom), cv::Scalar_<int>(255, 0, 0), 1);
                    }

                    for (auto e : ground_truth.objects) {
                        cv::rectangle(data.image, cv::Point2d(e.bbox.left, e.bbox.top), cv::Point2d(e.bbox.right, e.bbox.bottom), cv::Scalar_<int>(0, 255, 0), 2);
                    }

                    cv::imshow("undistorted_vs_gt", data.image);
                    cv::waitKey(0);
                }
            }

            // for (auto j = 0; j < std::min(undistorted_detections.objects.size(), distorted_detections.objects.size()); ++j) {
            //	auto undistorted_distorted_iou = iou(undistorted_detections.objects[j].bbox, distorted_detections.objects[j].bbox);
            //
            //	common::println(undistorted_distorted_iou);
            //}
        }
    }
}*/