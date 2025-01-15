#include "Yolo.h"

#include <torch/script.h>
#include <torch/torch.h>

using torch::indexing::None;
using torch::indexing::Slice;

/**
 * @brief Convert bounding box in format xyxy to format xywh.
 * @param x The input tensor which consists of bounding boxes for each detection.
 * @return The tesnor in xywh format.
 */
torch::Tensor xyxy2xywh(const torch::Tensor& x) {
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
torch::Tensor xywh2xyxy(const torch::Tensor& x) {
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
torch::Tensor nms(torch::Tensor const& bboxes, torch::Tensor const& scores, float const iou_threshold) {
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
torch::Tensor non_max_suppression(torch::Tensor& prediction, float conf_thres = 0.25, float iou_thres = 0.45, int max_det = 300) {
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
 * @tparam height The height of the scaled image placed in the Yolo detector.
 * @tparam width The width of the scaled image placed in the Yolo detector.
 * @return The scaled bounding boxes.
 */
template <int height, int width>
torch::Tensor scale_boxes(torch::Tensor& boxes, int const camera_height, int const camera_width) {
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
 * @brief Sets up the GPU and loads the yolo model and then performs a yolo inference.
 * @param input_image The downscaled input image.
 * @param camera_height The original height of the camera.
 * @param camera_width The original width of the camera.
 * @param model_path The path of the yolo model.
 * @tparam height The height of the scaled image placed in the Yolo detector.
 * @tparam width The width of the scaled image placed in the Yolo detector.
 * @tparam device_id The device on which yolo should run.
 */
template <int height, int width, int device_id>
std::vector<Detection2D> run_yolo(cv::Mat const& input_image, std::filesystem::path const& model_path, int const camera_height, int const camera_width) {
	thread_local static torch::Device device(torch::cuda::is_available() ? torch::kCUDA : torch::kCPU, device_id);
	thread_local static torch::jit::script::Module yolo_model = [&model_path] {
		common::println(torch::cuda::is_available() ? "GPU mode inference" : "CPU mode inference");

		torch::jit::script::Module yolo_model = torch::jit::load(model_path, device);
		yolo_model.eval();

		return yolo_model;
	}();

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
	keep.index_put_({Slice(), Slice(None, 4)}, scale_boxes<height, width>(boxes, camera_height, camera_width));

	std::vector<Detection2D> ret;
	for (int i = 0; i < keep.size(0); i++) {
		int const cls = keep[i][5].item().toInt();
		if (cls != 0 && cls != 1 && cls != 2 && cls != 3 && cls != 5 && cls != 7) continue;

		BoundingBoxXYXY const bbox{keep[i][0].item().toFloat(), keep[i][1].item().toFloat(), keep[i][2].item().toFloat(), keep[i][3].item().toFloat()};
		Detection2D const detection2D{bbox, keep[i][4].item().toFloat(), static_cast<std::uint8_t>(cls)};
		ret.emplace_back(detection2D);
	}

	return ret;
}

/**
 * @brief Defines run_yolo function for detector with size 640x640.
 */
template std::vector<Detection2D> run_yolo<640, 640, 0>(cv::Mat const& input_image, std::filesystem::path const& model_path, int camera_height, int camera_width);
template std::vector<Detection2D> run_yolo<640, 640, 1>(cv::Mat const& input_image, std::filesystem::path const& model_path, int camera_height, int camera_width);
template std::vector<Detection2D> run_yolo<640, 640, 2>(cv::Mat const& input_image, std::filesystem::path const& model_path, int camera_height, int camera_width);
template std::vector<Detection2D> run_yolo<640, 640, 3>(cv::Mat const& input_image, std::filesystem::path const& model_path, int camera_height, int camera_width);
/**
 * @brief Defines run_yolo function for detector with size 480x640.
 */
template std::vector<Detection2D> run_yolo<480, 640, 0>(cv::Mat const& input_image, std::filesystem::path const& model_path, int camera_height, int camera_width);
template std::vector<Detection2D> run_yolo<480, 640, 1>(cv::Mat const& input_image, std::filesystem::path const& model_path, int camera_height, int camera_width);
template std::vector<Detection2D> run_yolo<480, 640, 2>(cv::Mat const& input_image, std::filesystem::path const& model_path, int camera_height, int camera_width);
template std::vector<Detection2D> run_yolo<480, 640, 3>(cv::Mat const& input_image, std::filesystem::path const& model_path, int camera_height, int camera_width);