#include "image_communication_node.h"

#include <chrono>
#include <opencv2/opencv.hpp>
#include <thread>

#include "common_output.h"

GstRTSPServer *ImageStreamRTSP::_server = []() {
	gst_init(NULL, NULL);

	GstRTSPServer *server = gst_rtsp_server_new();
	gst_rtsp_server_set_service(server, port);

	return server;
}();
GstRTSPMountPoints *ImageStreamRTSP::_mounts = gst_rtsp_server_get_mount_points(_server);
GMainLoop *ImageStreamRTSP::_loop = g_main_loop_new(NULL, FALSE);

std::vector<ImageStreamRTSP *> ImageStreamRTSP::all_streams;

void ImageStreamRTSP::need_data(GstElement *appsrc, guint unused, StreamContext *context) {
	std::time_t time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	std::string wall_time = std::ctime(&time);

	std::shared_ptr<ImageData const> img = std::atomic_load(context->image);
	cv::putText(img->image, wall_time, cv::Point2d(500, 500), cv::FONT_HERSHEY_DUPLEX, 1.0, cv::Scalar_<int>(0, 0, 0), 1);

	guint size;
	GstBuffer *buffer;
	GstMapInfo m;

	if constexpr (use_jpeg) {
		std::vector<std::uint8_t> jpeg_buffer;
		cv::imencode(".jpeg", img->image, jpeg_buffer);

		size = jpeg_buffer.size();
		buffer = gst_buffer_new_allocate(NULL, size, NULL);

		gst_buffer_map(buffer, &m, GST_MAP_WRITE);
		std::memcpy(m.data, jpeg_buffer.data(), size);
	} else {
		size = 1920 * 1200 * 3;
		buffer = gst_buffer_new_allocate(NULL, size, NULL);

		gst_buffer_map(buffer, &m, GST_MAP_WRITE);
		std::memcpy(m.data, img->image.data, size);
	}

	gst_buffer_unmap(buffer, &m);

	GST_BUFFER_PTS(buffer) = context->timestamp;
	GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, 30);
	context->timestamp += GST_BUFFER_DURATION(buffer);

	GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(appsrc), buffer);
}

void ImageStreamRTSP::media_configure(GstRTSPMediaFactory *factory, GstRTSPMedia *media, ImageStreamRTSP *current_class) {
	GstElement *element, *appsrc;

	/* get the element used for providing the streams of the media */
	element = gst_rtsp_media_get_element(media);

	/* get our appsrc, we named it 'mysrc' with the name property */
	appsrc = gst_bin_get_by_name_recurse_up(GST_BIN(element), "mysrc");

	/* this instructs appsrc that we will be dealing with timed buffer */
	gst_util_set_object_arg(G_OBJECT(appsrc), "format", "time");
	/* configure the caps of the video */

	std::shared_ptr<ImageData const> image_data = std::atomic_load(&current_class->_image_data);
	if constexpr (use_jpeg) {
		g_object_set(G_OBJECT(appsrc), "caps", gst_caps_new_simple("image/jpeg", "width", G_TYPE_INT, image_data->width, "height", G_TYPE_INT, image_data->height, NULL), NULL);
	} else {
		g_object_set(G_OBJECT(appsrc), "caps", gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "BGR", "width", G_TYPE_INT, image_data->width, "height", G_TYPE_INT, image_data->height, NULL), NULL);
	}

	StreamContext *context = g_new0(StreamContext, 1);
	context->timestamp = 0;
	context->image = &current_class->_image_data;

	g_object_set_data_full(G_OBJECT(media), "my-extra-data", context, (GDestroyNotify)g_free);

	/* install the callback that will be called when a buffer is needed */
	g_signal_connect(appsrc, "need-data", (GCallback)ImageStreamRTSP::need_data, context);

	gst_element_set_state(element, GST_STATE_PLAYING);

	gst_object_unref(appsrc);
	gst_object_unref(element);
}

ImageStreamRTSP::ImageStreamRTSP() {
	all_streams.push_back(this);
	/* make a media factory for a test stream. The default media factory can use gst-launch syntax to create pipelines.
	 * any launch line works as long as it contains elements named pay%d. Each element with pay%d names will be a stream */
	_factory = gst_rtsp_media_factory_new();
	if constexpr (use_jpeg) {
		gst_rtsp_media_factory_set_launch(_factory, "( appsrc name=mysrc ! rtpjpegpay name=pay0 pt=26 )");
	} else {
		// gst_rtsp_media_factory_set_launch(_factory, "( appsrc name=mysrc ! videoconvert ! video/x-raw,format=I420 ! x264enc preset=ultrafast tune=zerolatency ! rtph264pay name=pay0 pt=96 )");
		gst_rtsp_media_factory_set_launch(_factory, "( appsrc name=mysrc ! videoconvert ! video/x-raw,format=I420 ! openh264enc multi-thread=4 complexity=low rate-control=buffer ! rtph264pay name=pay0 pt=96 )");
	}
	gst_rtsp_media_factory_set_shared(_factory, true);

	/* notify when our media is ready, This is called whenever someone asks for
	 * the media and a new pipeline with our appsrc is created */
	g_signal_connect(_factory, "media-configure", G_CALLBACK(media_configure), this);
}

void ImageStreamRTSP::run_loop() {
	// set mount point name with name of source
	for (auto stream : all_streams) {
		std::shared_ptr<ImageData const> image_data;

		do {
			image_data = std::atomic_load(&stream->_image_data);
			std::this_thread::yield();
		} while (!image_data);

		/* attach the factory to the url */
		gst_rtsp_mount_points_add_factory(_mounts, ("/" + image_data->source).c_str(), stream->_factory);
		common::println("[ImageStreamRTSP]: Stream ready at 'ffplay -fflags nobuffer -i \"rtsp://127.0.0.1:", port, "/", image_data->source, "\"'");
	}

	/* attach the server to the default maincontext */
	gst_rtsp_server_attach(_server, NULL);
	g_main_loop_run(_loop);
}

void ImageStreamRTSP::output_function(std::shared_ptr<ImageData const> const &data) { std::atomic_store(&_image_data, data); }