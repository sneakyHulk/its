/* GStreamer
 * Copyright (C) 2008 Wim Taymans <wim.taymans at gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <gst/app/gstappsrc.h>
#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>

#include <chrono>
#include <filesystem>
#include <opencv2/opencv.hpp>
#include <random>
#include <thread>
using namespace std::chrono_literals;

struct MyContext {
	static int i;
	int j;
	GstClockTime timestamp;
	std::atomic_bool need{false};
	GstElement *appsrc = nullptr;
};
int MyContext::i = 0;

static void push_data(MyContext *ctx) {
	while (true) {
		if (!ctx->need.load()) {
			std::cout << "wait!" << std::endl;
			std::this_thread::yield();
			continue;
		}

		std::time_t time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		std::string wall_time = std::ctime(&time);

		cv::Mat test_img = cv::imread(std::filesystem::path(CMAKE_SOURCE_DIR) / "data/camera_simulator/s110_o_cam_8/s110_o_cam_8_images_distorted/1690366190050.jpg");
		cv::putText(test_img, wall_time, cv::Point2d(500, 500), cv::FONT_HERSHEY_DUPLEX, 1.0, cv::Scalar_<int>(0, 0, 0), 1);

		GstBuffer *buffer;
		guint size = 1920 * 1200 * 3;
		buffer = gst_buffer_new_allocate(NULL, size, NULL);
		GstMapInfo m;
		gst_buffer_map(buffer, &m, GST_MAP_WRITE);
		memcpy(m.data, test_img.data, size);
		gst_buffer_unmap(buffer, &m);

		GST_BUFFER_PTS(buffer) = ctx->timestamp;
		GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, 15);
		ctx->timestamp += GST_BUFFER_DURATION(buffer);

		if (ctx->need.load()) {
			GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(ctx->appsrc), buffer);
		}
	}
}

/* called when we need to give data to appsrc */
static void need_data(GstElement *appsrc, guint unused, MyContext *ctx) {
	std::cout << "new image" << std::endl;

	std::time_t time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	std::string wall_time = std::ctime(&time);

	cv::Mat test_img = cv::imread(std::filesystem::path(CMAKE_SOURCE_DIR) / "data/camera_simulator/s110_o_cam_8/s110_o_cam_8_images_distorted/1690366190050.jpg");
	cv::putText(test_img, wall_time, cv::Point2d(500, 500), cv::FONT_HERSHEY_DUPLEX, 1.0, cv::Scalar_<int>(0, 0, 0), 1);
	cv::putText(test_img, std::to_string(ctx->j), cv::Point2d(500, 600), cv::FONT_HERSHEY_DUPLEX, 1.0, cv::Scalar_<int>(0, 0, 0), 1);

	GstBuffer *buffer;
	guint size;

	size = 1920 * 1200 * 3;
	buffer = gst_buffer_new_allocate(NULL, size, NULL);
	GstMapInfo m;
	gst_buffer_map(buffer, &m, GST_MAP_WRITE);
	memcpy(m.data, test_img.data, size);
	gst_buffer_unmap(buffer, &m);

	GST_BUFFER_PTS(buffer) = ctx->timestamp;
	GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, 15);
	ctx->timestamp += GST_BUFFER_DURATION(buffer);

	GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(ctx->appsrc), buffer);
}

/* called when a new media pipeline is constructed. We can query the
 * pipeline and configure our appsrc */
static void media_configure(GstRTSPMediaFactory *factory, GstRTSPMedia *media, gpointer user_data) {
	GstElement *element, *appsrc;
	MyContext *ctx;

	/* get the element used for providing the streams of the media */
	element = gst_rtsp_media_get_element(media);

	/* get our appsrc, we named it 'mysrc' with the name property */
	appsrc = gst_bin_get_by_name_recurse_up(GST_BIN(element), "mysrc");

	/* this instructs appsrc that we will be dealing with timed buffer */
	gst_util_set_object_arg(G_OBJECT(appsrc), "format", "time");
	/* configure the caps of the video */
	g_object_set(G_OBJECT(appsrc), "caps", gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "BGR", "width", G_TYPE_INT, 1920, "height", G_TYPE_INT, 1200, NULL), NULL);  // "framerate", GST_TYPE_FRACTION, 0, 1,

	ctx = g_new0(MyContext, 1);
	ctx->timestamp = 0;
	ctx->appsrc = appsrc;
	ctx->j = ++MyContext::i;
	/* make sure ther datais freed when the media is gone */
	g_object_set_data_full(G_OBJECT(media), "my-extra-data", ctx, (GDestroyNotify)g_free);

	/* install the callback that will be called when a buffer is needed */
	g_signal_connect(appsrc, "need-data", (GCallback)need_data, ctx);

	gst_element_set_state(element, GST_STATE_PLAYING);

	gst_object_unref(appsrc);
	gst_object_unref(element);
}

int main(int argc, char *argv[]) {
	GMainLoop *loop;
	GstRTSPServer *server;
	GstRTSPMountPoints *mounts;
	GstRTSPMediaFactory *factory;

	gst_init(&argc, &argv);

	loop = g_main_loop_new(NULL, FALSE);

	/* create a server instance */
	server = gst_rtsp_server_new();

	/* get the mount points for this server, every server has a default object
	 * that be used to map uri mount points to media factories */
	mounts = gst_rtsp_server_get_mount_points(server);

	/* make a media factory for a test stream. The default media factory can use
	 * gst-launch syntax to create pipelines.
	 * any launch line works as long as it contains elements named pay%d. Each
	 * element with pay%d names will be a stream */
	factory = gst_rtsp_media_factory_new();
	gst_rtsp_media_factory_set_launch(factory, "( appsrc name=mysrc ! videoconvert ! x264enc tune=zerolatency ! rtph264pay name=pay0 pt=96 )");

	/* notify when our media is ready, This is called whenever someone asks for
	 * the media and a new pipeline with our appsrc is created */
	g_signal_connect(factory, "media-configure", (GCallback)media_configure, NULL);

	/* attach the test factory to the /test url */
	gst_rtsp_mount_points_add_factory(mounts, "/test", factory);
	gst_rtsp_mount_points_add_factory(mounts, "/test2", factory);

	/* don't need the ref to the mounts anymore */
	g_object_unref(mounts);

	/* attach the server to the default maincontext */
	gst_rtsp_server_attach(server, NULL);

	/* start serving */
	g_print("stream ready at rtsp://127.0.0.1:8554/test\n");
	g_main_loop_run(loop);

	return 0;
}