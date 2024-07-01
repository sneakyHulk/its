#pragma once

#include <torch/script.h>
#include <torch/torch.h>

#include <filesystem>

#include "Detection2D.h"
#include "ImageData.h"
#include "node.h"

class Yolo final : public InputOutputNode<ImageData, Detections2D> {
	static std::array<std::string, 80> classes;

	torch::Device device;
	torch::jit::script::Module yolo_model;

   public:
	explicit Yolo(std::filesystem::path const& model_path = std::filesystem::path(CMAKE_SOURCE_DIR) / std::filesystem::path("data/yolo") / std::filesystem::path("yolov9c.torchscript"));

	Detections2D function(ImageData const& data) final;
};