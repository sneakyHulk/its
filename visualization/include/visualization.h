#pragma once

#include <boost/circular_buffer.hpp>

#include "camera/camera.h"
#include "common_output.h"
#include "msg/CompactObject.h"
#include "node/node.h"
#include "transformation/Config.h"

class ImageVisualization : public OutputNode<ImageData> {
	void output_function(ImageData const& data) final;
};

class Visualization2D : public OutputNode<CompactObjects> {
	Config const& config;
	cv::Mat map;
	Eigen::Matrix<double, 4, 4> utm_to_image;
	boost::circular_buffer<CompactObjects> drawing_fifo;

   public:
	explicit Visualization2D(Config const& config, std::filesystem::path const& map_path = std::filesystem::path(CMAKE_SOURCE_DIR) / std::filesystem::path("data/visualization/2021-07-07_1490_Providentia_Plus_Plus_1_6.xodr"));

   private:
	void output_function(CompactObjects const& data) final;
};