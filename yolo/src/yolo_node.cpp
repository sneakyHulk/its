#include "yolo_node.h"

#include <opencv2/opencv.hpp>
#include <stdexcept>

#include "common_output.h"

using torch::indexing::None;
using torch::indexing::Slice;

inline float generate_scale(cv::Mat const& image, const std::vector<int>& target_size) {
	int const origin_w = image.cols;
	int const origin_h = image.rows;

	int const target_h = target_size[0];
	int const target_w = target_size[1];

	float const ratio_h = static_cast<float>(target_h) / static_cast<float>(origin_h);
	float const ratio_w = static_cast<float>(target_w) / static_cast<float>(origin_w);
	float const resize_scale = std::min(ratio_h, ratio_w);
	return resize_scale;
}

inline float letterbox(cv::Mat const& input_image, cv::Mat& output_image, const std::vector<int>& target_size) {
	if (input_image.cols == target_size[1] && input_image.rows == target_size[0]) {
		if (input_image.data == output_image.data) {
			return 1.;
		} else {
			output_image = input_image.clone();
			return 1.;
		}
	}

	float const resize_scale = generate_scale(input_image, target_size);
	int const new_shape_w = static_cast<int>(std::round(static_cast<float>(input_image.cols) * resize_scale));
	int const new_shape_h = static_cast<int>(std::round(static_cast<float>(input_image.rows) * resize_scale));
	float const padw = static_cast<float>(target_size[1] - new_shape_w) / 2.f;
	float const padh = static_cast<float>(target_size[0] - new_shape_h) / 2.f;

	int const top = static_cast<int>(std::round(padh - 0.1));
	int const bottom = static_cast<int>(std::round(padh + 0.1));
	int const left = static_cast<int>(std::round(padw - 0.1));
	int const right = static_cast<int>(std::round(padw + 0.1));

	cv::resize(input_image, output_image, cv::Size(new_shape_w, new_shape_h), 0, 0, cv::INTER_AREA);

	cv::copyMakeBorder(output_image, output_image, top, bottom, left, right, cv::BORDER_CONSTANT, cv::Scalar(114.));
	return resize_scale;
}

inline torch::Tensor xyxy2xywh(const torch::Tensor& x) {
	auto y = torch::empty_like(x);
	y.index_put_({"...", 0}, (x.index({"...", 0}) + x.index({"...", 2})).div(2));
	y.index_put_({"...", 1}, (x.index({"...", 1}) + x.index({"...", 3})).div(2));
	y.index_put_({"...", 2}, x.index({"...", 2}) - x.index({"...", 0}));
	y.index_put_({"...", 3}, x.index({"...", 3}) - x.index({"...", 1}));
	return y;
}

inline torch::Tensor xywh2xyxy(const torch::Tensor& x) {
	auto y = torch::empty_like(x);
	auto const dw = x.index({"...", 2}).div(2);
	auto const dh = x.index({"...", 3}).div(2);
	y.index_put_({"...", 0}, x.index({"...", 0}) - dw);
	y.index_put_({"...", 1}, x.index({"...", 1}) - dh);
	y.index_put_({"...", 2}, x.index({"...", 0}) + dw);
	y.index_put_({"...", 3}, x.index({"...", 1}) + dh);
	return y;
}

// Reference: https://github.com/pytorch/vision/blob/main/torchvision/csrc/ops/cpu/nms_kernel.cpp
inline torch::Tensor nms(const torch::Tensor& bboxes, const torch::Tensor& scores, float iou_threshold) {
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

inline torch::Tensor non_max_suppression(torch::Tensor& prediction, float conf_thres = 0.25, float iou_thres = 0.45, int max_det = 300) {
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

inline torch::Tensor clip_boxes(torch::Tensor& boxes, const std::vector<int>& shape) {
	boxes.index_put_({"...", 0}, boxes.index({"...", 0}).clamp(0, shape[1]));
	boxes.index_put_({"...", 1}, boxes.index({"...", 1}).clamp(0, shape[0]));
	boxes.index_put_({"...", 2}, boxes.index({"...", 2}).clamp(0, shape[1]));
	boxes.index_put_({"...", 3}, boxes.index({"...", 3}).clamp(0, shape[0]));
	return boxes;
}

inline torch::Tensor scale_boxes(const std::vector<int>& img1_shape, torch::Tensor& boxes, const std::vector<int>& img0_shape) {
	auto gain = (std::min)((float)img1_shape[0] / img0_shape[0], (float)img1_shape[1] / img0_shape[1]);
	auto pad0 = std::round((float)(img1_shape[1] - img0_shape[1] * gain) / 2. - 0.1);
	auto pad1 = std::round((float)(img1_shape[0] - img0_shape[0] * gain) / 2. - 0.1);

	boxes.index_put_({"...", 0}, boxes.index({"...", 0}) - pad0);
	boxes.index_put_({"...", 2}, boxes.index({"...", 2}) - pad0);
	boxes.index_put_({"...", 1}, boxes.index({"...", 1}) - pad1);
	boxes.index_put_({"...", 3}, boxes.index({"...", 3}) - pad1);
	boxes.index_put_({"...", Slice(None, 4)}, boxes.index({"...", Slice(None, 4)}).div(gain));
	return boxes;
}

Yolo::Yolo(std::filesystem::path const& model_path) : device(torch::cuda::is_available() ? torch::kCUDA : torch::kCPU) {
	common::println(torch::cuda::is_available() ? "GPU mode inference" : "CPU mode inference");

	try {
		yolo_model = torch::jit::load(model_path, device);
		yolo_model.eval();
	} catch (const c10::Error& e) {
		throw std::runtime_error(e.msg());
	}
}

Detections2D Yolo::function(ImageData const& data) {
	Detections2D detections;
	try {
		cv::Mat input_image;
		letterbox(data.image, input_image, {640, 640});

		torch::Tensor image_tensor = torch::from_blob(input_image.data, {input_image.rows, input_image.cols, 3}, torch::kByte).to(device);
		image_tensor = image_tensor.toType(torch::kFloat32).div(255);
		image_tensor = image_tensor.permute({2, 0, 1});
		image_tensor = image_tensor.unsqueeze(0);
		std::vector<torch::jit::IValue> inputs{image_tensor};

		// inference
		torch::Tensor output = yolo_model.forward(inputs).toTensor().cpu();

		// NMS
		auto keep = non_max_suppression(output)[0];
		auto boxes = keep.index({Slice(), Slice(None, 4)});
		keep.index_put_({Slice(), Slice(None, 4)}, scale_boxes({input_image.rows, input_image.cols}, boxes, {data.image.rows, data.image.cols}));

		// paint the results
		for (int i = 0; i < keep.size(0); i++) {
			int cls = keep[i][5].item().toInt();
			if (cls != 0 && cls != 1 && cls != 2 && cls != 3 && cls != 5 && cls != 7) continue;

			BoundingBoxXYXY bbox{keep[i][0].item().toFloat(), keep[i][1].item().toFloat(), keep[i][2].item().toFloat(), keep[i][3].item().toFloat()};
			Detection2D detection{bbox, keep[i][4].item().toFloat(), static_cast<std::uint8_t>(cls)};

			detections.objects.push_back(detection);
		}

		detections.source = data.source;
		detections.timestamp = data.timestamp;
	} catch (const c10::Error& e) {
		common::println(e.msg());
	}

	// common::println("source: ", std::setw(13), data.source, " took ",
	//	    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::nanoseconds(std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now()).time_since_epoch().count() - data.timestamp)).count(), "ms
	//");

	return detections;
}

std::array<std::string, 80> Yolo::classes = {"person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat", "traffic light", "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat", "dog", "horse",
    "sheep", "cow", "elephant", "bear", "zebra", "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee", "skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove", "skateboard", "surfboard",
    "tennis racket", "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple", "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair", "couch", "potted plant", "bed",
    "dining table", "toilet", "tv", "laptop", "mouse", "remote", "keyboard", "cell phone", "microwave", "oven", "toaster", "sink", "refrigerator", "book", "clock", "vase", "scissors", "teddy bear", "hair drier", "toothbrush"};