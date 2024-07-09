#include <gst/app/gstappsrc.h>
#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>

#include <chrono>
#include <cstdint>
#include <opencv2/opencv.hpp>
#include <thread>
#include <vector>

#include "ImageData.h"
#include "camera_simulator_node.h"
#include "common_exception.h"

using namespace std::chrono_literals;

struct StreamContext {
	GstClockTime timestamp;
	std::shared_ptr<ImageData const> *image;
};

class [[maybe_unused]] ImageStreamRTSP : public OutputPtrNode<ImageData> {
	std::shared_ptr<ImageData const> _image_data;

	static GstRTSPServer *_server;
	static GstRTSPMountPoints *_mounts;
	GstRTSPMediaFactory *_factory;

   private:
	static void need_data(GstElement *appsrc, guint unused, StreamContext *context) {
		std::time_t time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		std::string wall_time = std::ctime(&time);

		guint size = 1920 * 1200 * 3;
		GstBuffer *buffer = gst_buffer_new_allocate(NULL, size, NULL);
		std::shared_ptr<ImageData const> img = std::atomic_load(context->image);

		GstMapInfo m;
		gst_buffer_map(buffer, &m, GST_MAP_WRITE);
		std::memcpy(m.data, img->image.data, size);

		// use buffer from memcpy
		cv::Mat image(1200, 1920, CV_8UC3, m.data);
		cv::putText(image, wall_time, cv::Point2d(500, 500), cv::FONT_HERSHEY_DUPLEX, 1.0, cv::Scalar_<int>(0, 0, 0), 1);

		gst_buffer_unmap(buffer, &m);

		GST_BUFFER_PTS(buffer) = context->timestamp;
		GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, 15);
		context->timestamp += GST_BUFFER_DURATION(buffer);

		GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(appsrc), buffer);
	}

	static void media_configure(GstRTSPMediaFactory *factory, GstRTSPMedia *media, ImageStreamRTSP *current_class) {
		GstElement *element, *appsrc;

		/* get the element used for providing the streams of the media */
		element = gst_rtsp_media_get_element(media);

		/* get our appsrc, we named it 'mysrc' with the name property */
		appsrc = gst_bin_get_by_name_recurse_up(GST_BIN(element), "mysrc");

		/* this instructs appsrc that we will be dealing with timed buffer */
		gst_util_set_object_arg(G_OBJECT(appsrc), "format", "time");
		/* configure the caps of the video */
		g_object_set(G_OBJECT(appsrc), "caps", gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "BGR", "width", G_TYPE_INT, 1920, "height", G_TYPE_INT, 1200, NULL), NULL);  // "framerate", GST_TYPE_FRACTION, 0, 1,

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

   public:
	ImageStreamRTSP(std::string const &stream_name = "/test") {
		/* make a media factory for a test stream. The default media factory can use
		 * gst-launch syntax to create pipelines.
		 * any launch line works as long as it contains elements named pay%d. Each
		 * element with pay%d names will be a stream */
		_factory = gst_rtsp_media_factory_new();
		gst_rtsp_media_factory_set_launch(_factory, "( appsrc name=mysrc ! videoconvert ! x264enc tune=zerolatency ! rtph264pay name=pay0 pt=96 )");
		//gst_rtsp_media_factory_set_profiles(_factory, GST_RTSP_PROFILE_AVPF);

		/* notify when our media is ready, This is called whenever someone asks for
		 * the media and a new pipeline with our appsrc is created */
		g_signal_connect(_factory, "media-configure", G_CALLBACK(media_configure), this);

		/* attach the test factory to the /test url */
		gst_rtsp_mount_points_add_factory(_mounts, stream_name.c_str(), _factory);

		/* attach the server to the default maincontext */
		gst_rtsp_server_attach(_server, NULL);

		g_print("stream ready at rtsp://127.0.0.1:%s%s\n", "8554", stream_name.c_str());
	}

   private:
	void output_function(std::shared_ptr<ImageData const> const &data) final { std::atomic_store(&_image_data, data); }
};

/* create a server instance */
GstRTSPServer *ImageStreamRTSP::_server = gst_rtsp_server_new();
/* get the mount points for this server, every server has a default object
 * that be used to map uri mount points to media factories */
GstRTSPMountPoints *ImageStreamRTSP::_mounts = _mounts = gst_rtsp_server_get_mount_points(_server);

int main(int argc, char **argv) {
	gst_init(&argc, &argv);
	GMainLoop *loop = g_main_loop_new(NULL, FALSE);

	CameraSimulator cam_n("s110_n_cam_8");
	CameraSimulator cam_o("s110_o_cam_8");
	ImageStreamRTSP transmitter("/test");
	ImageStreamRTSP transmitter2("/test2");

	cam_n += transmitter;
	cam_o += transmitter2;

	std::thread cam_n_thread(&CameraSimulator::operator(), &cam_n);
	std::thread cam_o_thread(&CameraSimulator::operator(), &cam_o);
	std::thread transmitter_thread(&ImageStreamRTSP::operator(), &transmitter);
	std::thread transmitter2_thread(&ImageStreamRTSP::operator(), &transmitter2);

	g_main_loop_run(loop);
}