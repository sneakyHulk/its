#pragma once

#include <filesystem>
#include <map>

#include "Detection2D.h"
#include "ImageData.h"
#include "Processor.h"
#include "Yolo.h"

/**
 * @class YoloNode
 * @brief This class performs the Yolo detection.
 * @tparam height The height of the scaled image placed in the Yolo detector.
 * @tparam width The width of the scaled image placed in the Yolo detector.
 * @tparam device_id The device on which yolo should run.
 */
template <int height, int width, int device_id = 0>
class YoloNode : public Processor<ImageData, Detections2D> {
	inline static std::array<std::string, 80> classes = {"person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat", "traffic light", "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat", "dog",
	    "horse", "sheep", "cow", "elephant", "bear", "zebra", "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee", "skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove", "skateboard",
	    "surfboard", "tennis racket", "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple", "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair", "couch", "potted plant",
	    "bed", "dining table", "toilet", "tv", "laptop", "mouse", "remote", "keyboard", "cell phone", "microwave", "oven", "toaster", "sink", "refrigerator", "book", "clock", "vase", "scissors", "teddy bear", "hair drier", "toothbrush"};

	struct CameraHeightWidthConfig {
		int camera_height;
		int camera_width;
	};

	std::map<std::string, CameraHeightWidthConfig> const camera_name_height_width;
	std::filesystem::path const model_path;

   public:
	/**
	 * @param camera_name_width_height A map that maps the names of the cameras connected to this node to the original sizes of these cameras.
	 * @param model_path The path of the yolo model.
	 */
	explicit YoloNode(std::map<std::string, CameraHeightWidthConfig>&& camera_name_height_width,
	    std::filesystem::path&& model_path = std::filesystem::path(CMAKE_SOURCE_DIR) / "data" / "yolo" / (std::to_string(height) + 'x' + std::to_string(width)) / "yolov9c.torchscript")
	    : camera_name_height_width(std::forward<decltype(camera_name_height_width)>(camera_name_height_width)), model_path(std::forward<decltype(model_path)>(model_path)) {}

	/**
	 * @brief Performs a yolo detection.
	 * @param data The image data to be used for detection.
	 * @return The detection result.
	 */
	Detections2D process(ImageData const& data) final {
		Detections2D detections;
		detections.source = data.source;
		detections.timestamp = data.timestamp;

		detections.objects = run_yolo<height, width, device_id>(data.image, camera_name_height_width.at(data.source).camera_height, camera_name_height_width.at(data.source).camera_width, model_path);

		return detections;
	}
};