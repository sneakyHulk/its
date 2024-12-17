#pragma once

#include <torch/script.h>
#include <torch/torch.h>

#include <filesystem>
#include <map>

#include "Detection2D.h"
#include "ImageData.h"
#include "Processor.h"

/**
 * @class YoloNode
 * @brief This class performs the Yolo detection.
 * @tparam height The height of the scaled image placed in the Yolo detector.
 * @tparam width The width of the scaled image placed in the Yolo detector.
 */
template <int height, int width>
class YoloNode : public Processor<ImageData, Detections2D> {
	static std::array<std::string, 80> classes;

	torch::Device device;
	torch::jit::script::Module yolo_model;

	struct CameraWidthHeightConfig {
		int camera_width;
		int camera_height;
	};

	std::map<std::string, CameraWidthHeightConfig> const camera_name_width_height;

   public:
	explicit YoloNode(std::map<std::string, CameraWidthHeightConfig>&& camera_name_width_height, int device_id = 0,
	    std::filesystem::path const& model_path = std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "yolo" / (std::to_string(height) + 'x' + std::to_string(width)) / "yolov9c.torchscript");

	Detections2D process(ImageData const& data) final;

   private:
	static torch::Tensor xyxy2xywh(torch::Tensor const& x);
	static torch::Tensor xywh2xyxy(torch::Tensor const& x);
	static torch::Tensor nms(torch::Tensor const& bboxes, torch::Tensor const& scores, float iou_threshold);
	static torch::Tensor non_max_suppression(torch::Tensor& prediction, float conf_thres = 0.25, float iou_thres = 0.45, int max_det = 300);
	cv::Mat letterbox(cv::Mat const& input_image) const;
	torch::Tensor scale_boxes(torch::Tensor& boxes, int camera_height, int camera_width) const;
};