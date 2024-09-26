#pragma once

#include "AfterReturnTimeMeasure.h"
#include "Detection2D.h"
#include "ImageData.h"
#include "node.h"
#include "yolo_function.h"

template <int height, int width>
class YoloNode : public InputOutputNode<ImageData, Detections2D> {
	// inline torch::Tensor scale_boxes(heght_small, width_small, torch::Tensor& boxes, cam.heght, cam.width) {
	//	auto gain = (std::min)((float)img1_shape[0] / img0_shape[0], (float)img1_shape[1] / img0_shape[1]);
	//	auto pad0 = std::round((float)(img1_shape[1] - img0_shape[1] * gain) / 2. - 0.1);
	//	auto pad1 = std::round((float)(img1_shape[0] - img0_shape[0] * gain) / 2. - 0.1);
	//
	//	boxes.index_put_({"...", 0}, boxes.index({"...", 0}) - pad0);
	//	boxes.index_put_({"...", 2}, boxes.index({"...", 2}) - pad0);
	//	boxes.index_put_({"...", 1}, boxes.index({"...", 1}) - pad1);
	//	boxes.index_put_({"...", 3}, boxes.index({"...", 3}) - pad1);
	//	boxes.index_put_({"...", Slice(None, 4)}, boxes.index({"...", Slice(None, 4)}).div(gain));
	//	return boxes;
	//}

   public:
	YoloNode() = default;

	Detections2D function(ImageData const& data) final {
		AfterReturnTimeMeasure after(data.timestamp);
		return run_yolo<height, width>(data);
	}
};