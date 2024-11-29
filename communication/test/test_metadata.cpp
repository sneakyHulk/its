#include <gst/app/gstappsrc.h>
#include <gst/gst.h>

#include <iostream>
#include <mutex>
#include <opencv2/opencv.hpp>
#include <string>
#include <thread>

GMainLoop *loopVideoCapture;
GMainLoop *loopVideoStream;

GstElement *Stream_pipeline, *Stream_appsrc, *Stream_conv, *Stream_videosink, *Stream_videoenc, *Stream_mux, *Stream_payloader, *Stream_filter;
GstCaps *convertCaps;

static cv::VideoCapture images("/home/lukas/Downloads/4865386-uhd_4096_2160_25fps.mp4");

#define AUTO_VIDEO_SINK 0
#define OLD_ALGO 0
#define APPSRC_WIDTH static_cast<int>(images.get(cv::CAP_PROP_FRAME_WIDTH))
#define APPSRC_HEIGHT static_cast<int>(images.get(cv::CAP_PROP_FRAME_HEIGHT))
#define APPSRC_FRAMERATE 30
std::mutex mtx;

struct FrameData {
	char *data;
	unsigned int size;
};

FrameData frameData;

void fill_appsrc(const unsigned char *DataPtr, gsize size) { memcpy(frameData.data, DataPtr, size); }

static void cb_need_data(GstElement *appsrc, guint unused_size, gpointer user_data) {
	static gboolean white = FALSE;
	static GstClockTime timestamp = 0;

	guint size, depth, height, width, step, channels;
	GstFlowReturn ret;
	cv::Mat frame;
	images >> frame;
	GstMapInfo map;
	mtx.lock();

	FrameData *frameData = (FrameData *)user_data;
	GstBuffer *buffer = NULL;  // gst_buffer_new_allocate (NULL, size, NULL);
	//
	//      g_print("frame_size: %d \n",size);
	//      g_print("timestamp: %ld \n",timestamp);

	cv::putText(frame, std::to_string(timestamp), cv::Point2d(500, 500), cv::FONT_HERSHEY_DUPLEX, 5.0, cv::Scalar_<int>(0, 0, 0), 5);

	buffer = gst_buffer_new_allocate(NULL, frame.total() * frame.elemSize(), NULL);
	gst_buffer_map(buffer, &map, GST_MAP_WRITE);
	memcpy((guchar *)map.data, frame.data, frame.total() * frame.elemSize());

	GST_BUFFER_PTS(buffer) = timestamp;
	GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, APPSRC_FRAMERATE);

	timestamp += GST_BUFFER_DURATION(buffer);
	// gst_app_src_push_buffer ((GstAppSrc *)appsrc, buffer);

	g_signal_emit_by_name(appsrc, "push-buffer", buffer, &ret);
	mtx.unlock();
}

void VideoStream(void) {
	gst_init(nullptr, nullptr);
	loopVideoStream = g_main_loop_new(NULL, FALSE);

	Stream_pipeline = gst_pipeline_new("pipeline");
	Stream_appsrc = gst_element_factory_make("appsrc", "source");
	Stream_conv = gst_element_factory_make("videoconvert", "conv");

#if (AUTO_VIDEO_SINK == 1)
	Stream_videosink = gst_element_factory_make("autovideosink", "videosink");

	gst_bin_add_many(GST_BIN(Stream_pipeline), Stream_appsrc, Stream_conv, Stream_videosink, NULL);
	gst_element_link_many(Stream_appsrc, Stream_conv, Stream_videosink, NULL);

#else

	Stream_videosink = gst_element_factory_make("udpsink", "videosink");
	g_object_set(G_OBJECT(Stream_videosink), "host", "127.0.0.1", "port", 5000, NULL);
	Stream_videoenc = gst_element_factory_make("x265enc", "videoenc");
	g_object_set(G_OBJECT(Stream_videoenc), "option-string", "repeat-headers=yes", NULL);
	g_object_set(G_OBJECT(Stream_videoenc), "bitrate", 200, NULL);

#if (OLD_ALGO == 1)
	Stream_payloader = gst_element_factory_make("rtpmp4vpay", "payloader");
	g_object_set(G_OBJECT(Stream_payloader), "config-interval", 60, NULL);

	gst_bin_add_many(GST_BIN(Stream_pipeline), Stream_appsrc, Stream_conv, Stream_videoenc, Stream_payloader, Stream_videosink, NULL);
	gst_element_link_many(Stream_appsrc, Stream_conv, Stream_videoenc, Stream_payloader, Stream_videosink, NULL);

#else

	Stream_filter = gst_element_factory_make("capsfilter", "converter-caps");
	Stream_payloader = gst_element_factory_make("rtph265pay", "rtp-payloader");

	convertCaps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "I420", "width", G_TYPE_INT, APPSRC_WIDTH, "height", G_TYPE_INT, APPSRC_HEIGHT, "framerate", GST_TYPE_FRACTION, APPSRC_FRAMERATE, 1, NULL);

	g_object_set(G_OBJECT(Stream_filter), "caps", convertCaps, NULL);

	gst_bin_add_many(GST_BIN(Stream_pipeline), Stream_appsrc, Stream_conv, Stream_filter, Stream_videoenc, Stream_payloader, Stream_videosink, NULL);
	gst_element_link_many(Stream_appsrc, Stream_conv, Stream_filter, Stream_videoenc, Stream_payloader, Stream_videosink, NULL);
#endif

#endif

	g_object_set(G_OBJECT(Stream_appsrc), "caps",
	    gst_caps_new_simple(
	        "video/x-raw", "format", G_TYPE_STRING, "RGB", "width", G_TYPE_INT, APPSRC_WIDTH, "height", G_TYPE_INT, APPSRC_HEIGHT, "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1, "framerate", GST_TYPE_FRACTION, APPSRC_FRAMERATE, 1, NULL),
	    NULL);

	g_object_set(G_OBJECT(Stream_appsrc), "stream-type", 0, "is-live", TRUE, "block", FALSE, "format", GST_FORMAT_TIME, NULL);

	g_signal_connect(Stream_appsrc, "need-data", G_CALLBACK(cb_need_data), &frameData);

	gst_element_set_state(Stream_pipeline, GST_STATE_PLAYING);
	g_main_loop_run(loopVideoStream);

	gst_element_set_state(Stream_pipeline, GST_STATE_NULL);
	gst_object_unref(Stream_pipeline);
}

GstFlowReturn NewFrame(GstElement *sink, void *data) {
	GstSample *sample = nullptr;
	GstBuffer *buffer = nullptr;
	GstMapInfo map;
	g_signal_emit_by_name(sink, "pull-sample", &sample);

	if (!sample) {
		g_printerr("Error: Could not obtain sample.\n");
		return GST_FLOW_ERROR;
	}
	//  printf("NewFrame\n");

	buffer = gst_sample_get_buffer(sample);
	gst_buffer_map(buffer, &map, GST_MAP_READ);

	mtx.lock();
	fill_appsrc(map.data, map.size);
	mtx.unlock();

	gst_buffer_unmap(buffer, &map);
	gst_sample_unref(sample);

	return GST_FLOW_OK;
}

void VideoCapture(void) {
	GstElement *pipeline, *source, *videorate, *testsrccaps, *jpegencode, *mjpegcaps, *decodemjpef, *convertmjpeg, *rgbcaps, *convertrgb, *sink;

	gst_init(nullptr, nullptr);

	pipeline = gst_pipeline_new("new-pipeline");
	source = gst_element_factory_make("videotestsrc", "source");
	videorate = gst_element_factory_make("videorate", "video-rate");
	testsrccaps = gst_element_factory_make("capsfilter", "test-src-caps");
	jpegencode = gst_element_factory_make("jpegenc", "jpeg-encoder");
	mjpegcaps = gst_element_factory_make("capsfilter", "mjpegcaps");
	decodemjpef = gst_element_factory_make("jpegdec", "decodemjpef");
	convertmjpeg = gst_element_factory_make("videoconvert", "convertmjpeg");
	rgbcaps = gst_element_factory_make("capsfilter", "rgbcaps");
	convertrgb = gst_element_factory_make("videoconvert", "convertrgb");
	sink = gst_element_factory_make("appsink", "sink");

	//  g_object_set(source, "device", "/dev/video0", NULL);

	g_object_set(G_OBJECT(testsrccaps), "caps", gst_caps_new_simple("video/x-raw", "framerate", GST_TYPE_FRACTION, 30, 1, NULL), NULL);

	g_object_set(G_OBJECT(mjpegcaps), "caps", gst_caps_new_simple("image/jpeg", "width", G_TYPE_INT, APPSRC_WIDTH, "height", G_TYPE_INT, APPSRC_HEIGHT, "framerate", GST_TYPE_FRACTION, 30, 1, NULL), NULL);

	g_object_set(G_OBJECT(rgbcaps), "caps", gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "RGB", "framerate", GST_TYPE_FRACTION, 30, 1, NULL), NULL);

	g_object_set(sink, "emit-signals", TRUE, "sync", TRUE, NULL);
	g_signal_connect(sink, "new-sample", G_CALLBACK(NewFrame), nullptr);

	gst_bin_add_many(GST_BIN(pipeline), source, testsrccaps, videorate, mjpegcaps, jpegencode, decodemjpef, convertmjpeg, rgbcaps, convertrgb, sink, NULL);
	if (!gst_element_link_many(source, testsrccaps, videorate, jpegencode, mjpegcaps, decodemjpef, convertmjpeg, rgbcaps, convertrgb, sink, NULL)) {
		g_printerr("Error: Elements could not be linked.\n");
	}

	gst_element_set_state(pipeline, GST_STATE_PLAYING);
	loopVideoCapture = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(loopVideoCapture);

	gst_element_set_state(pipeline, GST_STATE_NULL);
	gst_object_unref(pipeline);
}

int main(void) {
	frameData.size = APPSRC_WIDTH * APPSRC_HEIGHT * 4;
	frameData.data = new char[frameData.size];

	std::thread ThreadStreame(VideoStream);
	std::thread ThreadCapture(VideoCapture);

	while (1)
		;
	g_main_loop_quit(loopVideoCapture);
	g_main_loop_quit(loopVideoStream);
	ThreadStreame.join();
	ThreadCapture.join();

	return true;
}

/*#include <gst/app/gstappsrc.h>
#include <gst/gst.h>
#include <thread>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <chrono>

int main(int argc, char* argv[]) {
    gst_init(&argc, &argv);

    // OpenCV frame storage
    cv::Mat frame;

    // Create GStreamer elements
    GstElement* pipeline = gst_pipeline_new("pipeline");
    GstElement* appsrc = gst_element_factory_make("appsrc", "source");
    GstElement* capsfilter = gst_element_factory_make("capsfilter", "filter");
    GstElement* rtpvrawpay = gst_element_factory_make("rtpvrawpay", "payloader");
    GstElement* udpsink = gst_element_factory_make("udpsink", "sink");

    if (!pipeline || !appsrc || !capsfilter || !rtpvrawpay || !udpsink) {
        std::cerr << "Failed to create GStreamer elements" << std::endl;
        return -1;
    }

    // Set properties for appsrc
    g_object_set(G_OBJECT(appsrc), "format", GST_FORMAT_TIME, "is-live", TRUE, "do-timestamp", TRUE, nullptr);

    // Configure capsfilter with video format
    GstCaps* caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "BGR", "width", G_TYPE_INT, 640, "height", G_TYPE_INT, 480, "framerate", GST_TYPE_FRACTION, 30, 1, nullptr);
    g_object_set(G_OBJECT(capsfilter), "caps", caps, NULL);
    gst_caps_unref(caps);

    // Configure udpsink
    g_object_set(G_OBJECT(udpsink), "host", "127.0.0.1", "port", 5000, nullptr);

    // Build the pipeline
    gst_bin_add_many(GST_BIN(pipeline), appsrc, capsfilter, rtpvrawpay, udpsink, nullptr);
    if (!gst_element_link_many(appsrc, capsfilter, rtpvrawpay, udpsink, nullptr)) {
        std::cerr << "Failed to link elements" << std::endl;
        gst_object_unref(pipeline);
        return -1;
    }

    // Start the pipeline
    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "Failed to start the pipeline" << std::endl;
        gst_object_unref(pipeline);
        return -1;
    }

    // Frame generation and pushing loop
    std::cout << "Streaming frames to UDP (127.0.0.1:5000)" << std::endl;
    GstClockTime timestamp = 0;
    for (int i = 0; i < 300; ++i) {  // Stream for ~10 seconds (300 frames at 30 fps)
        // Create a dummy frame (or update your OpenCV frame here)
        frame = cv::Mat::zeros(cv::Size(640, 480), CV_8UC3);  // Black frame
        cv::putText(frame, "Hello, GStreamer!", cv::Point(50, 250), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 0), 2);

        // Convert frame to GStreamer buffer
        GstBuffer* buffer = gst_buffer_new_allocate(nullptr, frame.total() * frame.elemSize(), nullptr);
        GstMapInfo map;
        gst_buffer_map(buffer, &map, GST_MAP_WRITE);
        std::memcpy(map.data, frame.data, frame.total() * frame.elemSize());
        gst_buffer_unmap(buffer, &map);

        // Set the buffer's timestamp
        GST_BUFFER_PTS(buffer) = timestamp;
        GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, 30);  // 30 fps
        timestamp += GST_BUFFER_DURATION(buffer);

        // Push the buffer into the pipeline
        GstFlowReturn flow_ret = gst_app_src_push_buffer(GST_APP_SRC(appsrc), buffer);
        if (flow_ret != GST_FLOW_OK) {
            std::cerr << "Error pushing buffer to appsrc" << std::endl;
            break;
        }

        // Wait to simulate a 30 fps frame rate
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }

    // Clean up
    gst_element_send_event(pipeline, gst_event_new_eos());  // Send End-Of-Stream event
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    return 0;
}*/

/*#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include <gst/gst.h>

#include <cstring>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <thread>

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);

    cv::VideoCapture images("/home/lukas/Nextcloud/Documents/TUM/Presentations/combination_view_video.mp4");

    if (!images.isOpened()) {
        std::cerr << "Error: Cannot open video file" << std::endl;
        return -1;
    }

    int frameWidth = static_cast<int>(images.get(cv::CAP_PROP_FRAME_WIDTH));
    int frameHeight = static_cast<int>(images.get(cv::CAP_PROP_FRAME_HEIGHT));
    int frameCount = static_cast<int>(images.get(cv::CAP_PROP_FRAME_COUNT));

    // Create elements
    GstElement *pipeline = gst_pipeline_new("minimal-pipeline");
    GstElement *appsrc = gst_element_factory_make("appsrc", "source");
    GstElement *videoconvert1 = gst_element_factory_make("videoconvert", "converter1");
    GstElement *capsfilter1 = gst_element_factory_make("capsfilter", "capsfilter1");
    GstElement *encoder = gst_element_factory_make("x264enc", "encoder");
    GstElement *payloader = gst_element_factory_make("rtph264pay", "payloder");
    GstElement *sink = gst_element_factory_make("udpsink", "sink");
    // GstElement *decoder = gst_element_factory_make("avdec_h264", "decoder");
    // GstElement *sink = gst_element_factory_make("waylandsink", "sink");
    // GstElement *videoconvert2 = gst_element_factory_make("videoconvert", "converter2");
    // GstElement *capsfilter2 = gst_element_factory_make("capsfilter", "capsfilter2");
    // GstElement *sink = gst_element_factory_make("appsink", "sink");
    // GstElement *sink = gst_element_factory_make("", "sink");

    if (!pipeline || !appsrc || !videoconvert1 || !capsfilter1 || !encoder || !payloader || !sink) {  //|| !decoder || !videoconvert2 || !capsfilter2 || !sink) {
        std::cerr << "Failed to create elements!" << std::endl;
        return -1;
    }

    // Add elements to pipeline
    gst_bin_add_many(GST_BIN(pipeline), appsrc, videoconvert1, capsfilter1, encoder, payloader, sink, NULL);  // decoder, videoconvert2, capsfilter2, sink, NULL);

    // Link elements
    if (!gst_element_link_many(appsrc, videoconvert1, capsfilter1, encoder, payloader, sink, NULL)) {  // decoder, videoconvert2, capsfilter2, sink, NULL)) {
        std::cerr << "Failed to link elements!" << std::endl;
        gst_object_unref(pipeline);
        return -1;
    }

    // Configure appsrc
    GstCaps *appsrc_caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "BGR", "width", G_TYPE_INT, frameWidth, "height", G_TYPE_INT, frameHeight, NULL);
    g_object_set(G_OBJECT(appsrc), "caps", appsrc_caps, "emit-signals", FALSE, "format", GST_FORMAT_TIME, "leaky-type", 2, NULL);
    gst_caps_unref(appsrc_caps);

    GstCaps *conv_caps1 = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "I420", "width", G_TYPE_INT, frameWidth, "height", G_TYPE_INT, frameHeight, NULL);
    g_object_set(G_OBJECT(capsfilter1), "caps", conv_caps1, NULL);
    gst_caps_unref(conv_caps1);

    g_object_set(G_OBJECT(encoder), "tune", 0x00000004, "speed-preset", 9, NULL);

    g_object_set(G_OBJECT(payloader), "config-interval", 1, "pt", 96, NULL);
    g_object_set(G_OBJECT(sink), "host", "127.0.0.1", "port", 5000, NULL);

    // GstCaps *conv_caps2 = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "BGR", NULL);
    // g_object_set(G_OBJECT(capsfilter2), "caps", conv_caps2, NULL);
    // gst_caps_unref(conv_caps2);

    // g_object_set(G_OBJECT(sink), "emit-signals", TRUE, "max-buffers", 1, "max-time", 200, NULL);

    // Start playing
    if (GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING); ret == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "Failed to set pipeline to PLAYING state!" << std::endl;
        gst_object_unref(pipeline);
        return -1;
    }

    // Generate and push video frames
    cv::Mat image;
    GstClockTime timestamp = gst_util_get_timestamp();
    for (int i = 0; i < frameCount; std::cout << ++i << std::endl) {  // Push 300 frames (10 seconds of video at 30 fps)
        timestamp = gst_util_get_timestamp();
        images.read(image);

        cv::putText(image, std::to_string(i), cv::Point2d(500, 500), cv::FONT_HERSHEY_DUPLEX, 5.0, cv::Scalar_<int>(0, 0, 0), 5);

        GstBuffer *send_buffer = gst_buffer_new_allocate(NULL, frameWidth * frameHeight * 3, NULL);
        // Set buffer timestamps
        GST_BUFFER_PTS(send_buffer) = gst_util_uint64_scale(i, GST_SECOND, 10);       // 30 FPS
        GST_BUFFER_DURATION(send_buffer) = gst_util_uint64_scale(1, GST_SECOND, 10);  // Duration of 1/30 second

        if (GstMapInfo map; gst_buffer_map(send_buffer, &map, GST_MAP_WRITE)) {
            std::memcpy(map.data, image.data, frameWidth * frameHeight * 3);
            gst_buffer_unmap(send_buffer, &map);
        }

        // Push buffer to appsrc
        if (GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(appsrc), send_buffer); ret != GST_FLOW_OK) {
            std::cerr << "Error pushing buffer to appsrc!" << std::endl;
            break;
        }

        // if (GstSample *sample = gst_app_sink_try_pull_sample(GST_APP_SINK(sink), 10000000); sample != nullptr) {
        //	GstCaps *receive_caps = gst_sample_get_caps(sample);
        //	auto structure = gst_caps_get_structure(GST_CAPS(receive_caps), 0);
        //	int width, height;
        //	gst_structure_get_int(structure, "width", &width);
        //	gst_structure_get_int(structure, "height", &height);
        //	std::cout << "Sample: W = " << width << ", H = " << height << std::endl;
        //
        //	GstBuffer *receive_buffer = gst_sample_get_buffer(sample);
        //	GstMapInfo map;
        //
        //	if (!gst_buffer_map(receive_buffer, &map, GST_MAP_READ)) {
        //		std::cerr << "Error with mapping the gstreamer buffer" << std::endl;
        //		break;
        //	}
        //
        //	cv::Mat bgr_img(height, width, CV_8UC3, map.data);
        //	cv::Mat bgr_img2 = bgr_img.clone();
        //
        //	cv::imshow("image", bgr_img2);
        //	cv::waitKey(1);
        //
        //	gst_buffer_unmap(receive_buffer, &map);
        //	gst_sample_unref(sample);
        //}

        if (auto difference = gst_util_get_timestamp() - timestamp; gst_util_uint64_scale(1, GST_SECOND, 10) > difference) {
            std::cout << gst_util_uint64_scale(1, GST_SECOND, 10) - difference << std::endl;

            std::this_thread::sleep_for(std::chrono::nanoseconds(gst_util_uint64_scale(1, GST_SECOND, 10) - difference));
        }
    }

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    return 0;
}*/
/*
#include <gst/app/gstappsrc.h>
#include <gst/gst.h>

#include <opencv2/opencv.hpp>

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);

    // Load single image
    cv::Mat frame = cv::imread("/home/lukas/Downloads/Viktoria Kiesel.jpg");
    if (frame.empty()) {
        g_printerr("Could not load image\n");
        return -1;
    }

    GstElement *pipeline = gst_pipeline_new("stream");
    GstElement *source = gst_element_factory_make("appsrc", "source");
    GstElement *videoconvert = gst_element_factory_make("videoconvert", "converter");
    GstElement *capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
    GstElement *queue1 = gst_element_factory_make("queue", "queue1");
    GstElement *encoder = gst_element_factory_make("x264enc", "encoder");
    GstElement *decoder = gst_element_factory_make("avdec_h264", "decoder");
    GstElement *queue2 = gst_element_factory_make("queue", "queue2");
    GstElement *sink = gst_element_factory_make("autovideosink", "autovideosink");

    if (!pipeline || !source || !videoconvert || !capsfilter || !queue1 || !encoder || !decoder || !queue2 || !sink) {
        std::cerr << "Failed to create one or more elements!" << std::endl;
        return -1;
    }

    // Caps for raw video
    GstCaps *raw_caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "BGR", "width", G_TYPE_INT, frame.cols, "height", G_TYPE_INT, frame.rows, "framerate", GST_TYPE_FRACTION, 30, 1, NULL);
    g_object_set(G_OBJECT(source), "caps", raw_caps, "format", GST_FORMAT_TIME, NULL);

    GstCaps *conv_caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "I420", "width", G_TYPE_INT, frame.cols, "height", G_TYPE_INT, frame.rows, "framerate", GST_TYPE_FRACTION, 30, 1, NULL);
    g_object_set(G_OBJECT(capsfilter), "caps", conv_caps, NULL);

    g_object_set(G_OBJECT(encoder), "tune", 0, "speed-preset", 2, NULL);

    gst_bin_add_many(GST_BIN(pipeline), source, videoconvert, capsfilter, queue1, encoder, decoder, queue2, sink, NULL);
    gst_element_link_many(source, videoconvert, capsfilter, queue1, encoder, decoder, queue2, sink, NULL);

    if (GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING); ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Failed to set pipeline to PLAYING state.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    GstClockTime timestamp = 0;
    while (true) {
        guint size = frame.rows * frame.cols * 3;
        GstBuffer *buffer = gst_buffer_new_allocate(NULL, size, NULL);

        GST_BUFFER_PTS(buffer) = GST_CLOCK_TIME_NONE;
        GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, 10);

        if (GstMapInfo map; gst_buffer_map(buffer, &map, GST_MAP_WRITE)) {
            std::memcpy(map.data, frame.data, size);
            gst_buffer_unmap(buffer, &map);
        }

        if (GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(source), buffer); ret != GST_FLOW_OK) {
            std::cerr << "Error pushing buffer to appsrc!" << std::endl;
            break;
        }

        g_main_context_iteration(NULL, false);

        g_usleep(1000000);
    }

    return 0;
}*/
// GstElement *rtph264pay = gst_element_factory_make("rtph264pay", "rtp-payload");
// GstElement *udpsink = gst_element_factory_make("udpsink", "sink");

// g_object_set(G_OBJECT(openh264enc), NULL);  // Adjust bitrate to 500 kbps and use 'fast' preset

// g_object_set(G_OBJECT(rtph264pay), "config-interval", 1, "pt", 96, NULL);

// g_object_set(G_OBJECT(udpsink), "host", "127.0.0.1", "port", 5000, NULL);

// #include <gst/gst.h>
// #include <gst/app/gstappsrc.h>
// #include <opencv2/opencv.hpp>
// #include <iostream>
// #include <vector>
// #include <string.h>
// #include <sstream>
//
// int main(int argc, char *argv[]) {
// 	gst_init(&argc, &argv);
//
// 	// Load a single image using OpenCV
// 	cv::Mat image = cv::imread("/home/lukas/Downloads/Viktoria Kiesel.jpg"); // Replace with your image path
// 	if (image.empty()) {
// 		std::cerr << "Failed to load image!" << std::endl;
// 		return -1;
// 	}
//
// 	// Create GStreamer pipeline elements
// 	GstElement *pipeline = gst_pipeline_new("image-pipeline");
// 	GstElement *appsrc = gst_element_factory_make("appsrc", "image-source");
// 	GstElement *x264enc = gst_element_factory_make("x264enc", "h264-encoder");
// 	GstElement *rtppay = gst_element_factory_make("rtph264pay", "rtp-payloader");
// 	GstElement *udpsink = gst_element_factory_make("udpsink", "udp-sink");
//
// 	if (!pipeline || !appsrc || !x264enc || !rtppay || !udpsink) {
// 		std::cerr << "Failed to create GStreamer elements!" << std::endl;
// 		return -1;
// 	}
//
// 	// Configure UDP sink
// 	g_object_set(udpsink, "host", "127.0.0.1", "port", 5000, NULL);
//
// 	// Configure appsrc
// 	GstCaps *caps = gst_caps_new_simple("video/x-raw",
// 	    "format", G_TYPE_STRING, "BGR",
// 	    "width", G_TYPE_INT, image.cols,
// 	    "height", G_TYPE_INT, image.rows,
// 	    NULL);
// 	gst_app_src_set_caps(GST_APP_SRC(appsrc), caps);
// 	gst_caps_unref(caps);
//
// 	// Add elements to the pipeline
// 	gst_bin_add_many(GST_BIN(pipeline), appsrc, x264enc, rtppay, udpsink, NULL);
//
// 	// Link elements
// 	if (!gst_element_link_many(appsrc, x264enc, rtppay, udpsink, NULL)) {
// 		std::cerr << "Failed to link elements in the pipeline!" << std::endl;
// 		gst_object_unref(pipeline);
// 		return -1;
// 	}
//
// 	// Set pipeline to PLAYING
// 	// Start the pipeline
// 	GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
// 	if (ret == GST_STATE_CHANGE_FAILURE) {
// 		g_printerr("Failed to set pipeline to PLAYING state.\n");
// 		gst_object_unref(pipeline);
// 		return -1;
// 	}
//
//
// 	guint64 frame_count = 0;
// 	guint64 timestamp = 0;
// 	// Loop to push the same image repeatedly
// 	while (true) {
// 		frame_count = frame_count + 1;
// 		timestamp = timestamp + 1;
//
// 		guint8 sei_header[] = {
// 		    0x00, 0x00, 0x01, 0x06, // Start code and SEI NAL unit type
// 		};
//
// 		// Metadata: 8 bytes for frame count and 8 bytes for timestamp
// 		guint8 metadata[16];
// 		std::memcpy(metadata, &frame_count, sizeof(frame_count));
// 		std::memcpy(metadata + 8, &timestamp, sizeof(timestamp));
//
// 		// Allocate buffer for SEI + frame
// 		guint sei_size = sizeof(sei_header) + sizeof(metadata);
// 		guint total_size = sei_size + image.cols * image.rows * 3;
// 		GstBuffer *buffer = gst_buffer_new_allocate(NULL, total_size, NULL);
//
// 		// Fill SEI + frame into buffer
// 		GstMapInfo map;
// 		gst_buffer_map(buffer, &map, GST_MAP_WRITE);
// 		std::memcpy(map.data, sei_header, sizeof(sei_header));
// 		std::memcpy(map.data + sizeof(sei_header), metadata, sizeof(metadata));
// 		std::memcpy(map.data + sei_size, image.data, image.cols * image.rows * 3);
// 		gst_buffer_unmap(buffer, &map);
//
//
// 		// Set timestamps and duration for the buffer
// 		//GST_BUFFER_PTS(buffer) = timestamp;
// 		//GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale(1, GST_SECOND, 30);
//
// 		// Push buffer to appsrc
// 		GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(appsrc), buffer);
//
// 		if (ret != GST_FLOW_OK) {
// 			std::cerr << "Error pushing buffer to appsrc!" << std::endl;
// 			break;
// 		}
//
// 		g_main_context_iteration(NULL, false);
//
// 		// Release the buffer after pushing it
// 		// gst_buffer_unref(buffer);
//
// 		// Sleep to simulate frame rate
// 		g_usleep(1000000 / 30); // 30 FPS
// 	}
//
// 	// Cleanup
// 	gst_element_set_state(pipeline, GST_STATE_NULL);
// 	gst_object_unref(pipeline);
//
// 	return 0;
// }

/*#include <gst/gst.h>
#include <iostream>

// Define the FrameMeta structure
typedef struct _FrameMeta {
    GstMeta meta;
    guint64 frame_number;
    guint64 timestamp;
} FrameMeta;

const GstMetaInfo *frame_meta_get_info(void);

#define FRAME_META_API_TYPE (frame_meta_api_get_type())
#define FRAME_META_INFO (frame_meta_get_info())

GType frame_meta_api_get_type(void) {
    static GType type = 0;
    if (g_once_init_enter(&type)) {
        static const gchar *tags[] = { "frame-meta", NULL };
        GType _type = gst_meta_api_type_register("FrameMetaAPI", tags);
        g_once_init_leave(&type, _type);
    }
    return type;
}

static gboolean frame_meta_init(GstMeta *meta, gpointer params, GstBuffer *buffer) {
    FrameMeta *frame_meta = (FrameMeta *)meta;
    frame_meta->frame_number = 0; // Initialize frame number
    frame_meta->timestamp = 0;   // Initialize timestamp
    return TRUE;
}

const GstMetaInfo *frame_meta_get_info(void) {
    static const GstMetaInfo *info = NULL;
    if (g_once_init_enter(&info)) {
        const GstMetaInfo *_info = gst_meta_register(
            FRAME_META_API_TYPE,
            "FrameMeta",
            sizeof(FrameMeta),
            frame_meta_init,  // Init function
            NULL,             // Free function
            NULL              // Transform function
        );
        g_once_init_leave(&info, _info);
    }
    return info;
}

void add_frame_metadata(GstBuffer *buffer, guint64 frame_number, guint64 timestamp) {
    FrameMeta *meta = (FrameMeta *)gst_buffer_add_meta(buffer, FRAME_META_INFO, NULL);
    if (meta) {
        meta->frame_number = frame_number;
        meta->timestamp = timestamp;
    }
}

// Probe callback to add metadata to each frame
static GstPadProbeReturn add_metadata_to_frame(GstPad *pad, GstPadProbeInfo *info, gpointer user_data) {
    static guint64 frame_count = 0;

    if (GST_PAD_PROBE_INFO_TYPE(info) & GST_PAD_PROBE_TYPE_BUFFER) {
        GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);
        if (!buffer) {
            return GST_PAD_PROBE_OK;
        }

        // Add metadata
        guint64 timestamp = GST_BUFFER_PTS(buffer); // Use PTS as the timestamp
        add_frame_metadata(buffer, frame_count++, timestamp);
    }

    return GST_PAD_PROBE_OK;
}

// Probe callback to extract metadata
static GstPadProbeReturn extract_metadata_from_frame(GstPad *pad, GstPadProbeInfo *info, gpointer user_data) {
    if (GST_PAD_PROBE_INFO_TYPE(info) & GST_PAD_PROBE_TYPE_BUFFER) {
        GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);
        if (!buffer) {
            return GST_PAD_PROBE_OK;
        }

        // Extract metadata
        FrameMeta *meta = (FrameMeta *)gst_buffer_get_meta(buffer, FRAME_META_API_TYPE);
        if (meta) {
            std::cout << "Frame Number: " << meta->frame_number
                      << ", Timestamp: " << meta->timestamp << std::endl;
        }
    }

    return GST_PAD_PROBE_OK;
}

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);

    // Create pipeline elements
    GstElement *pipeline = gst_pipeline_new("test-pipeline");
    GstElement *source = gst_element_factory_make("videotestsrc", "source");
    GstElement *videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
    GstElement *sink = gst_element_factory_make("fakesink", "sink");

    if (!pipeline || !source || !videoconvert || !sink) {
        g_printerr("Failed to create elements.\n");
        return -1;
    }

    // Set the fakesink to silent mode to avoid verbose output
    g_object_set(sink, "sync", TRUE, "silent", TRUE, NULL);

    // Build the pipeline
    gst_bin_add_many(GST_BIN(pipeline), source, videoconvert, sink, NULL);
    if (!gst_element_link_many(source, videoconvert, sink, NULL)) {
        g_printerr("Failed to link elements.\n");
        return -1;
    }

    // Add probe to the source pad to add metadata
    GstPad *src_pad = gst_element_get_static_pad(source, "src");
    gst_pad_add_probe(src_pad, GST_PAD_PROBE_TYPE_BUFFER, add_metadata_to_frame, NULL, NULL);
    gst_object_unref(src_pad);

    // Add probe to the sink pad to extract metadata
    GstPad *sink_pad = gst_element_get_static_pad(sink, "sink");
    gst_pad_add_probe(sink_pad, GST_PAD_PROBE_TYPE_BUFFER, extract_metadata_from_frame, NULL, NULL);
    gst_object_unref(sink_pad);

    // Start the pipeline
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    // Run the main loop
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);

    // Clean up
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    g_main_loop_unref(loop);

    return 0;
}*/

/*
#include <gst/gst.h>
#include <iostream>




// Structure to hold custom metadata
typedef struct _FrameMeta {
    GstMeta meta;
    guint64 frame_number;
    guint64 timestamp;
} FrameMeta;

const GstMetaInfo *frame_meta_get_info(void);

#define FRAME_META_API_TYPE (frame_meta_api_get_type())
#define FRAME_META_INFO (frame_meta_get_info())

GType frame_meta_api_get_type(void) {
    static GType type = 0;
    if (g_once_init_enter(&type)) {
        // Provide a non-NULL tags array
        static const gchar *tags[] = {"frame-meta", NULL};
        GType _type = gst_meta_api_type_register("FrameMetaAPI", tags);
        g_once_init_leave(&type, _type);
    }
    return type;
}

// Meta initialization function
static gboolean frame_meta_init(GstMeta *meta, gpointer params, GstBuffer *buffer) {
    FrameMeta *frame_meta = (FrameMeta *)meta;
    frame_meta->frame_number = 0;  // Default value
    frame_meta->timestamp = 0;     // Default value
    return TRUE;
}

// Register the meta type
const GstMetaInfo *frame_meta_get_info(void) {
    static const GstMetaInfo *info = NULL;
    if (g_once_init_enter(&info)) {
        const GstMetaInfo *_info = gst_meta_register(FRAME_META_API_TYPE, "FrameMeta", sizeof(FrameMeta),
            frame_meta_init,                // Init function
            (GstMetaFreeFunction)NULL,      // Free function (optional)
            (GstMetaTransformFunction)NULL  // Transform function (optional)
        );
        g_once_init_leave(&info, _info);
    }
    return info;
}

void add_frame_metadata(GstBuffer *buffer, guint64 frame_number, guint64 timestamp) {
    FrameMeta *meta = (FrameMeta *)gst_buffer_add_meta(buffer, FRAME_META_INFO, NULL);
    if (meta) {
        meta->frame_number = frame_number;
        meta->timestamp = timestamp;
    }
}

// Probe callback to add metadata to each frame
static GstPadProbeReturn add_metadata_to_frame(GstPad *pad, GstPadProbeInfo *info, gpointer user_data) {
    static guint64 frame_count = 0;

    if (GST_PAD_PROBE_INFO_TYPE(info) & GST_PAD_PROBE_TYPE_BUFFER) {
        GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);
        if (!buffer) {
            return GST_PAD_PROBE_OK;
        }

        // Add metadata
        guint64 timestamp = GST_BUFFER_PTS(buffer);  // Use PTS as the timestamp
        add_frame_metadata(buffer, frame_count++, timestamp);
    }

    return GST_PAD_PROBE_OK;
}

// Probe callback to extract metadata
static GstPadProbeReturn extract_metadata_from_frame(GstPad *pad, GstPadProbeInfo *info, gpointer user_data) {
    if (GST_PAD_PROBE_INFO_TYPE(info) & GST_PAD_PROBE_TYPE_BUFFER) {
        GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);
        if (!buffer) {
            return GST_PAD_PROBE_OK;
        }

        // Extract metadata
        FrameMeta *meta = (FrameMeta *)gst_buffer_get_meta(buffer, FRAME_META_API_TYPE);
        if (meta) {
            std::cout << "Frame Number: " << meta->frame_number << ", Timestamp: " << meta->timestamp << std::endl;
        }
    }

    return GST_PAD_PROBE_OK;
}

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);

    // Create pipeline and elements
    GstElement *pipeline = gst_pipeline_new("sender-pipeline");

    GstElement *source = gst_element_factory_make("videotestsrc", "source");
    GstElement *capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
    GstElement *encoder = gst_element_factory_make("x264enc", "encoder");
    GstElement *payloader = gst_element_factory_make("rtph264pay", "payloader");
    GstElement *sink = gst_element_factory_make("udpsink", "sink");

    if (!pipeline || !source || !capsfilter || !encoder || !payloader || !sink) {
        g_printerr("One or more elements could not be created.\n");
        return -1;
    }

    g_object_set(G_OBJECT(source), "is-live", TRUE, "pattern", 0, NULL);

    GstCaps *caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "I420", "width", G_TYPE_INT, 640, "height", G_TYPE_INT, 480, "framerate", GST_TYPE_FRACTION, 30, 1, NULL);
    g_object_set(G_OBJECT(capsfilter), "caps", caps, NULL);
    gst_caps_unref(caps);

    g_object_set(G_OBJECT(encoder), "tune", 4, NULL);  // Tune for zero latency (zerolatency)

    g_object_set(G_OBJECT(sink), "host", "127.0.0.1", "port", 5600, NULL);

    gst_bin_add_many(GST_BIN(pipeline), source, capsfilter, encoder, payloader, sink, NULL);
    if (!gst_element_link_many(source, capsfilter, encoder, payloader, sink, NULL)) {
        g_printerr("Elements could not be linked.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    // Add probe to the source pad to add metadata
    GstPad *src_pad = gst_element_get_static_pad(sink, "sink");
    gst_pad_add_probe(src_pad, GST_PAD_PROBE_TYPE_BUFFER, add_metadata_to_frame, NULL, NULL);
    gst_object_unref(src_pad);

    // Start the pipeline
    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Failed to set pipeline to PLAYING state.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    // Run the main loop
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);

    // Clean up
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    g_main_loop_unref(loop);

    return 0;
}*/