#include "YoloNode.h"

#include <opencv2/opencv.hpp>

#include "common_output.h"

using torch::indexing::None;
using torch::indexing::Slice;

/**
 * @brief Convert bounding box in format xyxy to format xywh.
 * @param x The input tensor which consists of bounding boxes for each detection.
 * @return The tesnor in xywh format.
 */
template <int height, int width>
torch::Tensor YoloNode<height, width>::xyxy2xywh(const torch::Tensor& x) {
	auto y = torch::empty_like(x);
	y.index_put_({"...", 0}, (x.index({"...", 0}) + x.index({"...", 2})).div(2));
	y.index_put_({"...", 1}, (x.index({"...", 1}) + x.index({"...", 3})).div(2));
	y.index_put_({"...", 2}, x.index({"...", 2}) - x.index({"...", 0}));
	y.index_put_({"...", 3}, x.index({"...", 3}) - x.index({"...", 1}));
	return y;
}
/**
 * @brief Convert bounding box in format xywh to format xyxy.
 * @param x The input tensor which consists of bounding boxes for each detection.
 * @return The tesnor in xyxy format.
 */
template <int height, int width>
torch::Tensor YoloNode<height, width>::xywh2xyxy(const torch::Tensor& x) {
	auto y = torch::empty_like(x);
	auto const dw = x.index({"...", 2}).div(2);
	auto const dh = x.index({"...", 3}).div(2);
	y.index_put_({"...", 0}, x.index({"...", 0}) - dw);
	y.index_put_({"...", 1}, x.index({"...", 1}) - dh);
	y.index_put_({"...", 2}, x.index({"...", 0}) + dw);
	y.index_put_({"...", 3}, x.index({"...", 1}) + dh);
	return y;
}

/**
 * @brief Does the non-maximum suppression.
 *
 * @note Reference: https://github.com/pytorch/vision/blob/main/torchvision/csrc/ops/cpu/nms_kernel.cpp
 *
 * @param bboxes The bounding boxes of the detections.
 * @param scores The confidence scores of the detections.
 * @param iou_threshold The threshold value indicates that the iou overlap with a previous detection is too large and therefore the result of that detection is skipped.
 * @return The bounding boxes of the resulting detections along with their confidence and object class.
 */
template <int height, int width>
torch::Tensor YoloNode<height, width>::nms(torch::Tensor const& bboxes, torch::Tensor const& scores, float const iou_threshold) {
	if (bboxes.numel() == 0) return torch::empty({0}, bboxes.options().dtype(torch::kLong));

	auto const x1_t = bboxes.select(1, 0).contiguous();
	auto const y1_t = bboxes.select(1, 1).contiguous();
	auto const x2_t = bboxes.select(1, 2).contiguous();
	auto const y2_t = bboxes.select(1, 3).contiguous();

	torch::Tensor const areas_t = (x2_t - x1_t) * (y2_t - y1_t);

	auto const order_t = std::get<1>(scores.sort(/*stable=*/true, /*dim=*/0, /* descending=*/true));

	auto ndets = bboxes.size(0);
	torch::Tensor const suppressed_t = torch::zeros({ndets}, bboxes.options().dtype(torch::kByte));
	torch::Tensor const keep_t = torch::zeros({ndets}, bboxes.options().dtype(torch::kLong));

	auto const suppressed = suppressed_t.data_ptr<uint8_t>();
	auto const keep = keep_t.data_ptr<int64_t>();
	auto const order = order_t.data_ptr<int64_t>();
	auto const x1 = x1_t.data_ptr<float>();
	auto const y1 = y1_t.data_ptr<float>();
	auto const x2 = x2_t.data_ptr<float>();
	auto const y2 = y2_t.data_ptr<float>();
	auto const areas = areas_t.data_ptr<float>();

	int64_t num_to_keep = 0;

	for (int64_t _i = 0; _i < ndets; _i++) {
		auto const i = order[_i];
		if (suppressed[i] == 1) continue;
		keep[num_to_keep++] = i;
		auto ix1 = x1[i];
		auto iy1 = y1[i];
		auto ix2 = x2[i];
		auto iy2 = y2[i];
		auto const iarea = areas[i];

		for (int64_t _j = _i + 1; _j < ndets; _j++) {
			auto j = order[_j];
			if (suppressed[j] == 1) continue;
			auto const xx1 = std::max(ix1, x1[j]);
			auto const yy1 = std::max(iy1, y1[j]);
			auto const xx2 = std::min(ix2, x2[j]);
			auto const yy2 = std::min(iy2, y2[j]);

			auto const w = std::max(static_cast<float>(0), xx2 - xx1);
			auto const h = std::max(static_cast<float>(0), yy2 - yy1);
			auto const inter = w * h;
			auto const ovr = inter / (iarea + areas[j] - inter);
			if (ovr > iou_threshold) suppressed[j] = 1;
		}
	}
	return keep_t.narrow(0, 0, num_to_keep);
}

/**
 * @brief Prepares non-maximum suppression from yolo output.
 * @param prediction The predicted values coming from yolo.
 * @param conf_thres The threshold confidence value of a prediction below which the non-maximum suppression is not applied.
 * @param iou_thres The threshold value indicates that the iou overlap with a previous detection is too large and therefore the result of that detection is skipped.
 * @param max_det The value indicates the maximum number of results to be returned. Results with lesser confidence will be skipped.
 * @return The bounding boxes of the resulting detections along with their confidence and object class.
 */
template <int height, int width>
torch::Tensor YoloNode<height, width>::non_max_suppression(torch::Tensor& prediction, float const conf_thres, float const iou_thres, int const max_det) {
	auto const bs = prediction.size(0);
	auto nc = prediction.size(1) - 4;
	auto nm = prediction.size(1) - nc - 4;
	auto mi = 4 + nc;
	auto const xc = prediction.index({Slice(), Slice(4, mi)}).amax(1) > conf_thres;

	prediction = prediction.transpose(-1, -2);
	prediction.index_put_({"...", Slice({None, 4})}, xywh2xyxy(prediction.index({"...", Slice(None, 4)})));

	std::vector<torch::Tensor> output;
	for (int i = 0; i < bs; i++) {
		output.push_back(torch::zeros({0, 6 + nm}, prediction.device()));
	}

	for (int xi = 0; xi < prediction.size(0); xi++) {
		auto x = prediction[xi];
		x = x.index({xc[xi]});
		auto x_split = x.split({4, nc, nm}, 1);
		auto box = x_split[0], cls = x_split[1], mask = x_split[2];
		auto [conf, j] = cls.max(1, true);
		x = torch::cat({box, conf, j.toType(torch::kFloat), mask}, 1);
		x = x.index({conf.view(-1) > conf_thres});
		int n = x.size(0);
		if (!n) {
			continue;
		}

		// NMS
		auto c = x.index({Slice(), Slice{5, 6}}) * 7680;
		auto boxes = x.index({Slice(), Slice(None, 4)}) + c;
		auto scores = x.index({Slice(), 4});
		auto i = nms(boxes, scores, iou_thres);
		i = i.index({Slice(None, max_det)});
		output[xi] = x.index({i});
	}

	return torch::stack(output);
}

/**
 * @breif Scales the bounding boxes to the size of the original camera size to perform the right transformations.
 * @param boxes The bounding boxes to be scaled.
 * @param camera_height The height of the original image as it comes out of the camera.
 * @param camera_width The width of the original image as it comes out of the camera.
 * @return The scaled bounding boxes.
 */
template <int height, int width>
torch::Tensor YoloNode<height, width>::scale_boxes(torch::Tensor& boxes, int const camera_height, int const camera_width) const {
	auto const gain = (std::min)(static_cast<double>(height) / camera_height, static_cast<double>(width) / camera_width);
	auto const pad0 = std::round((width - camera_width * gain) / 2. - 0.1);
	auto const pad1 = std::round((height - camera_height * gain) / 2. - 0.1);

	boxes.index_put_({"...", 0}, boxes.index({"...", 0}) - pad0);
	boxes.index_put_({"...", 2}, boxes.index({"...", 2}) - pad0);
	boxes.index_put_({"...", 1}, boxes.index({"...", 1}) - pad1);
	boxes.index_put_({"...", 3}, boxes.index({"...", 3}) - pad1);
	boxes.index_put_({"...", Slice(None, 4)}, boxes.index({"...", Slice(None, 4)}).div(gain));
	return boxes;
}

/**
 * @brief Sets up the GPU and loads the yolo model.
 * @param camera_name_width_height A map that maps the names of the cameras connected to this node to the original sizes of these cameras.
 * @param device_id The device id of the GPU.
 * @param model_path The path of the yolo model.
 */
template <int height, int width>
YoloNode<height, width>::YoloNode(std::map<std::string, CameraWidthHeightConfig>&& camera_name_width_height, int device_id, std::filesystem::path const& model_path)
    : device(torch::cuda::is_available() ? torch::kCUDA : torch::kCPU, device_id), camera_name_width_height(std::forward<decltype(camera_name_width_height)>(camera_name_width_height)) {
	common::println_loc(torch::cuda::is_available() ? "GPU mode inference" : "CPU mode inference");

	try {
		yolo_model = torch::jit::load(model_path, device);
		yolo_model.eval();
	} catch (const c10::Error& e) {
		common::println_critical_loc(e.msg());
	}
}

/**
 * @brief Scales the image to the input size of the yolo model.
 *
 * Scales the image in such a way that the aspect ratio of the image is maintained.
 * Fills in the other parts with cv::BORDER_CONSTANT.
 *
 * @param input_image The image to be scaled.
 * @return The scaled image.
 */
template <int height, int width>
cv::Mat YoloNode<height, width>::letterbox(cv::Mat const& input_image) const {
	if (input_image.rows == height && input_image.cols == width) return input_image;

	cv::Mat ret;

	float const ratio_h = static_cast<float>(height) / static_cast<float>(input_image.rows);
	float const ratio_w = static_cast<float>(width) / static_cast<float>(input_image.cols);
	float const resize_scale = std::min(ratio_h, ratio_w);

	int const new_shape_w = static_cast<int>(std::round(static_cast<float>(input_image.cols) * resize_scale));
	int const new_shape_h = static_cast<int>(std::round(static_cast<float>(input_image.rows) * resize_scale));
	float const padw = static_cast<float>(width - new_shape_w) / 2.f;
	float const padh = static_cast<float>(height - new_shape_h) / 2.f;

	int const top = static_cast<int>(std::round(padh - 0.1));
	int const bottom = static_cast<int>(std::round(padh + 0.1));
	int const left = static_cast<int>(std::round(padw - 0.1));
	int const right = static_cast<int>(std::round(padw + 0.1));

	cv::resize(input_image, ret, cv::Size(new_shape_w, new_shape_h), 0, 0, cv::INTER_AREA);
	cv::copyMakeBorder(ret, ret, top, bottom, left, right, cv::BORDER_CONSTANT, cv::Scalar(114.));

	return ret;
}

/**
 * @brief Performs a yolo detection.
 * @param data The image data to be used for detection.
 * @return The detection result.
 */
template <int height, int width>
Detections2D YoloNode<height, width>::process(ImageData const& data) {
	Detections2D detections;
	detections.source = data.source;
	detections.timestamp = data.timestamp;

	try {
		cv::Mat input_image = letterbox(data.image);

		torch::Tensor image_tensor = torch::from_blob(input_image.data, {input_image.rows, input_image.cols, 3}, torch::kByte).to(device);
		image_tensor = image_tensor.toType(torch::kFloat32).div(255);
		image_tensor = image_tensor.permute({2, 0, 1});
		image_tensor = image_tensor.unsqueeze(0);
		std::vector<torch::jit::IValue> const inputs{image_tensor};

		// inference
		torch::Tensor output = yolo_model.forward(inputs).toTensor().cpu();

		auto keep = non_max_suppression(output)[0];

		// scales the boxes
		auto boxes = keep.index({Slice(), Slice(None, 4)});
		keep.index_put_({Slice(), Slice(None, 4)}, scale_boxes(boxes, camera_name_width_height.at(data.source).camera_height, camera_name_width_height.at(data.source).camera_width));

		for (int i = 0; i < keep.size(0); i++) {
			int const cls = keep[i][5].item().toInt();
			if (cls != 0 && cls != 1 && cls != 2 && cls != 3 && cls != 5 && cls != 7) continue;

			BoundingBoxXYXY const bbox{keep[i][0].item().toFloat(), keep[i][1].item().toFloat(), keep[i][2].item().toFloat(), keep[i][3].item().toFloat()};
			Detection2D detection{bbox, keep[i][4].item().toFloat(), static_cast<std::uint8_t>(cls)};

			detections.objects.push_back(detection);
		}
	} catch (const c10::Error& e) {
		common::println_critical_loc(e.msg());
	}

	return detections;
}

template <int height, int width>
std::array<std::string, 80> YoloNode<height, width>::classes = {"person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat", "traffic light", "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat",
    "dog", "horse", "sheep", "cow", "elephant", "bear", "zebra", "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee", "skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove", "skateboard",
    "surfboard", "tennis racket", "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple", "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair", "couch", "potted plant", "bed",
    "dining table", "toilet", "tv", "laptop", "mouse", "remote", "keyboard", "cell phone", "microwave", "oven", "toaster", "sink", "refrigerator", "book", "clock", "vase", "scissors", "teddy bear", "hair drier", "toothbrush"};

/**
 * @brief Defines YoloNode class for detector with size 640x640.
 */
template class YoloNode<640, 640>;
/**
 * @brief Defines YoloNode class for detector with size 480x640.
 */
template class YoloNode<480, 640>;