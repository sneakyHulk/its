#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include <gst/gst.h>

#include <bitset>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <string>
#include <thread>

#include "common_exception.h"
#include "common_output.h"

static GstFlowReturn new_sample(GstElement *sink, gpointer user_data) {
	GstSample *sample;
	g_signal_emit_by_name(sink, "pull-sample", &sample);

	if (sample) {
		// Process the sample here
		GstBuffer *buffer = gst_sample_get_buffer(sample);
		GstMapInfo map;

		if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
			// Access frame data in map.data, map.size
			std::cout << "Received frame of size: " << map.size << " bytes" << std::endl;
			gst_buffer_unmap(buffer, &map);
		}

		gst_sample_unref(sample);
		return GST_FLOW_OK;
	}

	return GST_FLOW_ERROR;
}

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

	GstElement *pipeline = gst_pipeline_new("rtsp-pipeline");
	GstElement *source = gst_element_factory_make("rtspsrc", "source");
	GstElement *depay = gst_element_factory_make("rtph264depay", "depay");
	GstElement *parse = gst_element_factory_make("h264parse", "parse");
	GstElement *decode = gst_element_factory_make("avdec_h264", "decode");
	GstElement *sink = gst_element_factory_make("appsink", "mysink");

	if (!pipeline || !source || !depay || !parse || !decode || !sink) {
		std::cerr << "Failed to create elements" << std::endl;
		return -1;
	}

	// Configure the source element
	const char *rtsp_url = "rtsp://80.155.138.138:2346/s60_n_cam_16_k";
	g_object_set(source, "location", rtsp_url, NULL);
	g_object_set(source, "latency", 10, NULL);

	// Configure the appsink element
	g_object_set(sink, "max-buffers", 2, NULL);
	g_object_set(sink, "sync", TRUE, NULL);
	g_object_set(sink, "emit-signals", TRUE, NULL);

	GstCaps *caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "I420", NULL);
	g_object_set(sink, "caps", caps, NULL);
	gst_caps_unref(caps);

	// Connect the new-sample signal to our callback
	// g_signal_connect(sink, "new-sample", G_CALLBACK(new_sample), NULL);

	// Assemble the pipeline
	gst_bin_add_many(GST_BIN(pipeline), source, depay, parse, decode, sink, NULL);

	// Link elements (rtspsrc requires special linking)
	if (!gst_element_link_many(depay, parse, decode, sink, NULL)) {
		std::cerr << "Failed to link elements" << std::endl;
		gst_object_unref(pipeline);
		return -1;
	}

	// Link the dynamic pad for rtspsrc
	g_signal_connect(source, "pad-added", G_CALLBACK(+[](GstElement *src, GstPad *new_pad, GstElement *data) {
		GstPad *sink_pad = gst_element_get_static_pad(data, "sink");
		if (gst_pad_is_linked(sink_pad)) {
			gst_object_unref(sink_pad);
			return;
		}
		if (gst_pad_link(new_pad, sink_pad) != GST_PAD_LINK_OK) {
			std::cerr << "Failed to link rtspsrc pad to depay" << std::endl;
		}
		gst_object_unref(sink_pad);
	}),
	    depay);

	// Start playing
	GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
	if (ret == GST_STATE_CHANGE_FAILURE) {
		std::cerr << "Failed to set pipeline to PLAYING" << std::endl;
		gst_object_unref(pipeline);
		return -1;
	}

	for (;;) {
		// Exit on EOS
		if (gst_app_sink_is_eos(GST_APP_SINK(sink))) {
			std::cout << "EOS !" << std::endl;
			break;
		}

		// Pull the sample (synchronous, wait)
		GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));
		if (sample == nullptr) {
			std::cout << "NO sample !" << std::endl;
			break;
		}

		// Get width and height from sample caps (NOT element caps)
		GstCaps *caps = gst_sample_get_caps(sample);
		auto structure = gst_caps_get_structure(GST_CAPS(caps), 0);
		int width, height;

		gst_structure_get_int(structure, "width", &width);
		gst_structure_get_int(structure, "height", &height);
		std::cout << "Sample: W = " << width << ", H = " << height << std::endl;

		GstBuffer *buffer = gst_sample_get_buffer(sample);
		GstMapInfo m;

		if (!gst_buffer_map(buffer, &m, GST_MAP_READ)) {
			throw common::Exception("Error with mapping the gstreamer buffer");
		}

		std::cout << "Received frame of size: " << m.size << " bytes" << std::endl;

		cv::Mat yuv_img(height + height / 2, width, CV_8UC1, m.data);
		cv::Mat bgr_img(height, width, CV_8UC3);

		cv::cvtColor(yuv_img, bgr_img, cv::COLOR_YUV2BGR_I420);

		std::uint64_t timestamp = get_image_based_timestamp(bgr_img);

		std::chrono::nanoseconds current_server_timestamp = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now()).time_since_epoch();
		std::chrono::nanoseconds image_creation_timestamp = std::chrono::nanoseconds(timestamp);

		common::println("timestamp: ", timestamp, ", e2e latency: ", current_server_timestamp - image_creation_timestamp);

		cv::imshow("frame", bgr_img);
		cv::waitKey(1);

		gst_buffer_unmap(buffer, &m);
		gst_sample_unref(sample);
	}

	// Run the main loop
	// GMainLoop *loop = g_main_loop_new(NULL, FALSE);
	// g_main_loop_run(loop);

	// Clean up
	gst_element_set_state(pipeline, GST_STATE_NULL);
	gst_object_unref(pipeline);
	// g_main_loop_unref(loop);

	return 0;
}

//======================================================================================================================
/// A simple assertion function + macro
inline void myAssert(bool b, const std::string &s = "MYASSERT ERROR !") {
	if (!b) throw std::runtime_error(s);
}

#define MY_ASSERT(x) myAssert(x, "MYASSERT ERROR :" #x)

//======================================================================================================================
/// Check GStreamer error, exit on error
inline void checkErr(GError *err) {
	if (err) {
		std::cerr << "checkErr : " << err->message << std::endl;
		exit(0);
	}
}

//======================================================================================================================
/// Our global data, serious gstreamer apps should always have this !
struct GoblinData {
	GstElement *pipeline = nullptr;
	GstElement *sinkVideo = nullptr;
};

//======================================================================================================================
/// Process a single bus message, log messages, exit on error, return false on eof
static bool busProcessMsg(GstElement *pipeline, GstMessage *msg, const std::string &prefix) {
	using namespace std;

	GstMessageType mType = GST_MESSAGE_TYPE(msg);
	cout << "[" << prefix << "] : mType = " << mType << " ";
	switch (mType) {
		case (GST_MESSAGE_ERROR):
			// Parse error and exit program, hard exit
			GError *err;
			gchar *dbg;
			gst_message_parse_error(msg, &err, &dbg);
			cout << "ERR = " << err->message << " FROM " << GST_OBJECT_NAME(msg->src) << endl;
			cout << "DBG = " << dbg << endl;
			g_clear_error(&err);
			g_free(dbg);
			exit(1);
		case (GST_MESSAGE_EOS):
			// Soft exit on EOS
			cout << " EOS !" << endl;
			return false;
		case (GST_MESSAGE_STATE_CHANGED):
			// Parse state change, print extra info for pipeline only
			cout << "State changed !" << endl;
			if (GST_MESSAGE_SRC(msg) == GST_OBJECT(pipeline)) {
				GstState sOld, sNew, sPenging;
				gst_message_parse_state_changed(msg, &sOld, &sNew, &sPenging);
				cout << "Pipeline changed from " << gst_element_state_get_name(sOld) << " to " << gst_element_state_get_name(sNew) << endl;
			}
			break;
		case (GST_MESSAGE_STEP_START): cout << "STEP START !" << endl; break;
		case (GST_MESSAGE_STREAM_STATUS): cout << "STREAM STATUS !" << endl; break;
		case (GST_MESSAGE_ELEMENT):
			cout << "MESSAGE ELEMENT !" << endl;
			break;

			// You can add more stuff here if you want

		default: cout << endl;
	}
	return true;
}

//======================================================================================================================
/// Run the message loop for one bus
void codeThreadBus(GstElement *pipeline, GoblinData &data, const std::string &prefix) {
	using namespace std;
	GstBus *bus = gst_element_get_bus(pipeline);

	int res;
	while (true) {
		GstMessage *msg = gst_bus_timed_pop(bus, GST_CLOCK_TIME_NONE);
		MY_ASSERT(msg);
		res = busProcessMsg(pipeline, msg, prefix);
		gst_message_unref(msg);
		if (!res) break;
	}
	gst_object_unref(bus);
	cout << "BUS THREAD FINISHED : " << prefix << endl;
}

//======================================================================================================================
/// Appsink process thread
void codeThreadProcessV(GoblinData &data) {
	using namespace std;
	for (;;) {
		// Exit on EOS
		if (gst_app_sink_is_eos(GST_APP_SINK(data.sinkVideo))) {
			cout << "EOS !" << endl;
			break;
		}

		// Pull the sample (synchronous, wait)
		GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(data.sinkVideo));
		if (sample == nullptr) {
			cout << "NO sample !" << endl;
			break;
		}

		// Get width and height from sample caps (NOT element caps)
		GstCaps *caps = gst_sample_get_caps(sample);
		MY_ASSERT(caps != nullptr);
		GstStructure *s = gst_caps_get_structure(caps, 0);
		int imW, imH;
		MY_ASSERT(gst_structure_get_int(s, "width", &imW));
		MY_ASSERT(gst_structure_get_int(s, "height", &imH));
		cout << "Sample: W = " << imW << ", H = " << imH << endl;

		//        cout << "sample !" << endl;
		// Process the sample
		// "buffer" and "map" are used to access raw data in the sample
		// "buffer" is a single data chunk, for raw video it's 1 frame
		// "buffer" is NOT a queue !
		// "Map" is the helper to access raw data in the buffer
		GstBuffer *buffer = gst_sample_get_buffer(sample);
		GstMapInfo m;
		MY_ASSERT(gst_buffer_map(buffer, &m, GST_MAP_READ));
		MY_ASSERT(m.size == imW * imH * 3);
		//        cout << "size = " << map.size << " ==? " << imW*imH*3 << endl;

		// Wrap the raw data in OpenCV frame and show on screen
		cv::Mat frame(imH, imW, CV_8UC3, (void *)m.data);

		int key = cv::waitKey(1);

		// Don't forget to unmap the buffer and unref the sample
		gst_buffer_unmap(buffer, &m);
		gst_sample_unref(sample);

		std::this_thread::sleep_for(std::chrono::seconds(1));

		cv::imshow("frame", frame);

		if (27 == key) exit(0);
	}
}

//======================================================================================================================
int main3(int argc, char **argv) {
	using namespace std;
	cout << "VIDEO1 : Send video to appsink, display with cv::imshow()" << endl;

	// Init gstreamer
	gst_init(&argc, &argv);

	// Our global data
	GoblinData data;

	// Set up the pipeline
	// Caps in appsink are important
	// max-buffers=2 to limit the queue and RAM usage
	// sync=1 for real-time playback, try sync=0 for fun !
	string pipeStr = "rtspsrc location=rtsp://127.0.0.1:2346/s110_n_cam_8 latency=50 ! rtph264depay ! h264parse ! avdec_h264 ! appsink name=mysink max-buffers=2 sync=1 caps=video/x-raw,format=I420";
	GError *err = nullptr;
	data.pipeline = gst_parse_launch(pipeStr.c_str(), &err);
	checkErr(err);
	MY_ASSERT(data.pipeline);
	// Find our appsink by name
	data.sinkVideo = gst_bin_get_by_name(GST_BIN(data.pipeline), "mysink");
	MY_ASSERT(data.sinkVideo);

	// Play the pipeline
	MY_ASSERT(gst_element_set_state(data.pipeline, GST_STATE_PLAYING));

	// Start the bus thread
	// thread threadBus([&data]() -> void {
	//	codeThreadBus(data.pipeline, data, "GOBLIN");
	//});

	// Start the appsink process thread
	thread threadProcess([&data]() -> void { codeThreadProcessV(data); });

	// Wait for threads
	// threadBus.join();
	threadProcess.join();

	// Destroy the pipeline
	gst_element_set_state(data.pipeline, GST_STATE_NULL);
	gst_object_unref(data.pipeline);

	return 0;
}

int main2(int argc, char *argv[]) {
	gst_init(&argc, &argv);

	// gst-launch-1.0 rtspsrc location=rtsp://127.0.0.1:2346/s110_n_cam_8 latency=50 ! rtph264depay ! h264parse ! avdec_h264 ! video/x-raw,format=I420 ! autovideosink
	// auto source = gst_element_factory_make("rtspsrc", "source");
	// auto decode_h264_rtp = gst_element_factory_make("rtph264depay", "decode_h264_rtp");
	// auto parse_h264 = gst_element_factory_make("h264parse", "parse_h264");
	// auto decode_h264 = gst_element_factory_make("avdec_h264", "decode_h264");
	// auto video_sink = gst_element_factory_make("autovideosink", "video_sink");
	// auto pipeline = gst_pipeline_new("test-pipeline");
	//
	// if (!pipeline || !source || !decode_h264_rtp || !parse_h264 || !decode_h264 || !video_sink) {
	//	g_printerr("Not all elements could be created.\n");
	//	return -1;
	//}
	//
	// g_object_set(source, "location", "rtsp://127.0.0.1:2346/s110_n_cam_8", "latency", 50, nullptr);
	// gst_bin_add_many(GST_BIN(pipeline), source, decode_h264_rtp, parse_h264, decode_h264, video_sink, nullptr);
	//
	// if (!gst_element_link_many(source, decode_h264_rtp, parse_h264, parse_h264, decode_h264, video_sink, nullptr)) {
	//	g_printerr("Elements could not be linked.\n");
	//	gst_object_unref(pipeline);
	//	return -1;
	//}

	// std::string pipeStr = "rtspsrc location=rtsp://127.0.0.1:2346/s110_n_cam_8 latency=50 ! rtph264depay ! h264parse ! avdec_h264 ! video/x-raw,format=I420 ! autovideosink";
	//
	// GError *err = nullptr;
	// auto pipeline = gst_parse_launch(pipeStr.c_str(), &err);
	// if (err) {
	// 	std::cerr << "checkErr : " << err->message << std::endl;
	// 	exit(0);
	// }
	//
	// gst_element_set_state(pipeline, GST_STATE_PLAYING);
	//
	// GstBus *bus = gst_element_get_bus(pipeline);
	//
	// while (true) {
	// 	GstMessage *msg = gst_bus_timed_pop(bus, GST_CLOCK_TIME_NONE);
	// 	if(!msg) {
	// 		throw std::runtime_error("no message");
	// 	}
	// 	gst_message_unref(msg);
	// }
	// gst_object_unref(bus);
	// std::cout << "BUS THREAD FINISHED : " << std::endl;

	// GstElement *src = gst_element_factory_make("videotestsrc", "goblin_src");
	// GstElement *conv = gst_element_factory_make("videoconvert", "goblin_conv");
	// GstElement *sink = gst_element_factory_make("autovideosink", "goblin_sink");
	// GstElement *pipeline = gst_pipeline_new("goblin_pipeline");
	// if (!src && !conv && !sink && !pipeline) {
	// 	g_printerr("Not all elements could be created.\n");
	// 	return -1;
	// }
	//
	// // Set up parameters if needed
	// g_object_set(src, "pattern", 18, nullptr);
	//
	// // Add and link elements
	// gst_bin_add_many(GST_BIN(pipeline), src, conv, sink, nullptr);
	// if (!gst_element_link_many(src, conv, sink, nullptr)) {
	// 	g_printerr("Elements could not be linked.\n");
	// 	gst_object_unref(pipeline);
	// 	return -1;
	// }
}