#include <gst/app/gstappsink.h>
#include <gst/gst.h>

#include <iostream>
#include <opencv2/opencv.hpp>
#include <chrono>

constexpr void set1(std::size_t index, std::uint64_t &value) { value |= 1ULL << index; }
constexpr void set0(std::size_t index, std::uint64_t &value) { value &= ~(1ULL << index); }
std::uint64_t get_image_based_timestamp(cv::Mat const &image) {
	std::uint64_t timestamp = 0;

	auto width = image.cols / 64;
	auto height = std::min(width, image.rows);

	for (auto i = 0; i < 64; ++i) {
		cv::Rect roi_mask(cv::Point(i * width, 0), cv::Point(i * width + width, height));
		cv::Mat roi = image(roi_mask);

		if (auto avg = cv::mean(roi); avg.val[0] + avg.val[1] + avg.val[2] > 127 * 3) {
			set1(i, timestamp);
		} else {
			set0(i, timestamp);
		}
	}

	return timestamp;
}

int main(int argc, char *argv[]) {
	gst_init(&argc, &argv);

	cv::Mat latestFrame;

	// Create pipeline and elements
	GstElement *pipeline = gst_pipeline_new("receiver-pipeline");

	GstElement *source = gst_element_factory_make("udpsrc", "source");
	GstElement *capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
	GstElement *depayloader = gst_element_factory_make("rtph264depay", "depayloader");
	GstElement *decoder = gst_element_factory_make("avdec_h264", "decoder");
	GstElement *videoconvert = gst_element_factory_make("videoconvert", "converter");
	GstElement *filter = gst_element_factory_make("capsfilter", "filter");
	GstElement *sink = gst_element_factory_make("appsink", "sink");

	if (!pipeline || !source || !capsfilter || !depayloader || !decoder || !videoconvert || !filter || !sink) {
		g_printerr("Failed to create elements.\n");
		return -1;
	}

	g_object_set(source, "port", 5000, NULL);

	GstCaps *caps = gst_caps_new_simple("application/x-rtp", "media", G_TYPE_STRING, "video", "encoding-name", G_TYPE_STRING, "H264", "payload", G_TYPE_INT, 96, NULL);
	g_object_set(G_OBJECT(capsfilter), "caps", caps, NULL);
	gst_caps_unref(caps);

	GstCaps *filter_caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "BGR", NULL);
	g_object_set(G_OBJECT(filter), "caps", filter_caps, NULL);
	gst_caps_unref(filter_caps);

	g_object_set(G_OBJECT(sink), "sync", FALSE, "max-buffers", 1, "drop", TRUE, NULL);  // "max-buffers", 5, "drop", TRUE

	gst_bin_add_many(GST_BIN(pipeline), source, capsfilter, depayloader, decoder, videoconvert, filter, sink, NULL);
	if (!gst_element_link_many(source, capsfilter, depayloader, decoder, videoconvert, filter, sink, NULL)) {
		g_printerr("Failed to link elements.\n");
		return -1;
	}

	// Start the pipeline
	if (GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING); ret == GST_STATE_CHANGE_FAILURE) {
		g_printerr("Failed to set pipeline to PLAYING state.\n");
		gst_object_unref(pipeline);
		return -1;
	}
	GstMapInfo map;
	GstBuffer *buffer;
	for (;;) {
		// Exit on EOS
		if (gst_app_sink_is_eos(GST_APP_SINK(sink))) {
			std::cout << "EOS !" << std::endl;
			break;
		}

		// Pull the sample (synchronous, wait)

		if (GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(sink)); sample != nullptr) {
			// Get width and height from sample caps (NOT element caps)
			GstCaps *caps = gst_sample_get_caps(sample);
			auto structure = gst_caps_get_structure(GST_CAPS(caps), 0);
			int width, height;

			gst_structure_get_int(structure, "width", &width);
			gst_structure_get_int(structure, "height", &height);
			auto format = gst_structure_get_string(structure, "format");
			std::cout << "Sample: W = " << width << ", H = " << height << "format: " << format << std::endl;

			buffer = gst_sample_get_buffer(sample);

			if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
				cv::Mat bgr_img(height, width, CV_8UC3, map.data);

				auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - get_image_based_timestamp(bgr_img);
				std::cout << std::chrono::nanoseconds(timestamp) << std::endl;

				cv::imshow("image", bgr_img);
				cv::waitKey(1);

				gst_buffer_unmap(buffer, &map);
			} else {
				std::cerr << "Error with mapping the gstreamer buffer" << std::endl;
			}

			gst_sample_unref(sample);
		} else {
			std::cout << "NO sample !" << std::endl;
			break;
		}
	}

	// Run the main loop
	// while (true) {
	// if (!latestFrame.empty()) {
	//	cv::imshow("Received Stream", latestFrame);
	// }

	// g_main_context_iteration(NULL, false);

	// if (GstSample *sample = gst_app_sink_try_pull_sample(GST_APP_SINK(sink), 10000000); sample != nullptr) {
	//	GstCaps *receive_caps = gst_sample_get_caps(sample);
	//	auto structure = gst_caps_get_structure(GST_CAPS(receive_caps), 0);
	//	int width, height;
	//	gst_structure_get_int(structure, "width", &width);
	//	gst_structure_get_int(structure, "height", &height);
	//	std::cout << "Sample: W = " << width << ", H = " << height << std::endl;
	//	GstBuffer *receive_buffer = gst_sample_get_buffer(sample);
	//	GstMapInfo map;
	//	if (!gst_buffer_map(receive_buffer, &map, GST_MAP_READ)) {
	//		std::cerr << "Error with mapping the gstreamer buffer" << std::endl;
	//		break;
	//	}
	//	cv::Mat bgr_img(height, width, CV_8UC3, map.data);
	//	std::cout << "bgr" << std::endl;
	//
	//	cv::Mat bgr_img2 = bgr_img.clone();
	//	std::cout << "bgr2" << std::endl;
	//
	//	cv::imshow("image", bgr_img2);
	//	cv::waitKey(1);
	//	gst_buffer_unmap(receive_buffer, &map);
	//	gst_sample_unref(sample);
	//}
	//}

	GMainLoop *loop = g_main_loop_new(nullptr, FALSE);
	g_main_loop_run(loop);

	// Clean up
	gst_element_set_state(pipeline, GST_STATE_NULL);
	gst_object_unref(pipeline);
	g_main_loop_unref(loop);

	return 0;
}