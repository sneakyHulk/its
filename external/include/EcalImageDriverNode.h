// siehe https://gitlab.lrz.de/providentiaplusplus/toolchain/-/blob/master/package/basler_camera/src/basler_camera.cpp

#pragma once
#include <ecal/ecal.h>

#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>
#include <chrono>
#include <csignal>
#include <opencv2/opencv.hpp>

#include "ImageDataRaw.h"
#include "common_output.h"
#include "node.h"
#include "streaming_message.pb.h"

// Yeah, the following code doesn't make much sense, but it is there for legacy reason and nobody wanted to change this except me and I didn't want to build upon this mess. Therefore, this class exists.
class EcalImageDriverNode : public OutputNode<ImageDataRaw> {
	struct UndistortionConfigInternal {
		cv::Mat camera_matrix;
		cv::Mat distortion_values;
		cv::Mat new_camera_matrix;
		cv::Mat undistortion_map1;
		cv::Mat undistortion_map2;
	};

	struct UndistortionConfigInternalPublisherConfig {
		UndistortionConfigInternal undistortion_config;
		eCAL::CPublisher publisher;
		int width;
		int height;
	};

   public:
	struct UndistortionConfig {
		std::vector<double> camera_matrix;
		std::vector<double> distortion_values;
		int width;
		int height;
	};

	explicit EcalImageDriverNode(std::map<std::string, UndistortionConfig>&& config) {
		for (auto const& [camera_name, undistortion_config] : config) {
			_undistortion_config_internal_publisher_map[camera_name].width = undistortion_config.width;
			_undistortion_config_internal_publisher_map[camera_name].height = undistortion_config.height;

			cv::Mat camera_matrix = cv::Mat_<double>(3, 3);
			for (auto i = 0; i < camera_matrix.rows; ++i) {
				for (auto j = 0; j < camera_matrix.cols; ++j) {
					camera_matrix.at<double>(i, j) = undistortion_config.camera_matrix[i * 3 + j];
				}
			}
			_undistortion_config_internal_publisher_map[camera_name].undistortion_config.camera_matrix = camera_matrix;

			cv::Mat distortion_values;
			for (auto i = 0; i < undistortion_config.distortion_values.size(); ++i) {
				distortion_values.push_back(undistortion_config.distortion_values[i]);
			}
			_undistortion_config_internal_publisher_map[camera_name].undistortion_config.distortion_values = distortion_values;

			cv::Size image_size(undistortion_config.width, undistortion_config.height);
			cv::Mat new_camera_matrix = cv::getOptimalNewCameraMatrix(
			    _undistortion_config_internal_publisher_map[camera_name].undistortion_config.camera_matrix, _undistortion_config_internal_publisher_map[camera_name].undistortion_config.distortion_values, image_size, 0., image_size);
			_undistortion_config_internal_publisher_map[camera_name].undistortion_config.new_camera_matrix = new_camera_matrix;

			cv::initUndistortRectifyMap(_undistortion_config_internal_publisher_map[camera_name].undistortion_config.camera_matrix, _undistortion_config_internal_publisher_map[camera_name].undistortion_config.distortion_values,
			    cv::Mat_<double>::eye(3, 3), _undistortion_config_internal_publisher_map[camera_name].undistortion_config.new_camera_matrix, image_size, CV_32F,
			    _undistortion_config_internal_publisher_map[camera_name].undistortion_config.undistortion_map1, _undistortion_config_internal_publisher_map[camera_name].undistortion_config.undistortion_map2);

			_undistortion_config_internal_publisher_map[camera_name].publisher.SetLayerMode(eCAL::TLayer::tlayer_shm, eCAL::TLayer::smode_on);
			_undistortion_config_internal_publisher_map[camera_name].publisher.Create(std::string("rgb_") + camera_name + "_pload_" + std::to_string(0));
		}
	}

   private:
	std::map<std::string, UndistortionConfigInternalPublisherConfig> _undistortion_config_internal_publisher_map;

	void output_function(ImageDataRaw const& data) final {
		if (eCAL::Ok()) {
			cv::Mat const bayer_image(_undistortion_config_internal_publisher_map.at(data.source).height, _undistortion_config_internal_publisher_map.at(data.source).width, CV_8UC1, const_cast<std::uint8_t*>(data.image_raw.data()));

			cv::Mat image_mat_rgb;
			cv::demosaicing(bayer_image, image_mat_rgb, cv::ColorConversionCodes::COLOR_BayerRG2RGB);
			cv::remap(image_mat_rgb, image_mat_rgb, _undistortion_config_internal_publisher_map[data.source].undistortion_config.undistortion_map1,
			    _undistortion_config_internal_publisher_map[data.source].undistortion_config.undistortion_map2, cv::INTER_LINEAR);

			providentia::StreamingMessage image_message_rgb;
			image_message_rgb.set_timestamp(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::nanoseconds(data.timestamp)).count());
			image_message_rgb.mutable_image()->set_data((const void*)image_mat_rgb.data, image_mat_rgb.total() * image_mat_rgb.elemSize());
			image_message_rgb.mutable_image()->set_encoding(providentia::Image::RGB24);
			std::string image_str = image_message_rgb.mutable_image()->data();
			_undistortion_config_internal_publisher_map[data.source].publisher.Send(image_str, int64_t(image_message_rgb.timestamp()));
		}
	}
};
