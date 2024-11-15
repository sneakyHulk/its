#pragma once

#include <gst/app/gstappsrc.h>
#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>

#include <thread>
#include <vector>

#include "ImageData.h"
#include "node.h"

struct StreamContext {
	GstClockTime timestamp;
	std::shared_ptr<ImageData const> *image;
};

class ImageStreamRTSP : public RunnerPtr<ImageData> {
	static constexpr bool use_jpeg = true;
	static const char *port;

	static GstRTSPServer *_server;
	static GstRTSPMountPoints *_mounts;
	static GMainLoop *_loop;

	static std::vector<ImageStreamRTSP *> all_streams;
	inline static std::thread _gstreamer_thread;

	std::shared_ptr<ImageData const> _image_data = nullptr;
	GstRTSPMediaFactory *_factory;
	std::function<bool(ImageData const &)> _image_mask;

   private:
	static void need_data(GstElement *appsrc, guint unused, StreamContext *context);
	static void media_configure(GstRTSPMediaFactory *factory, GstRTSPMedia *media, ImageStreamRTSP *current_class);
	[[noreturn]] void run_loop();
	static void add_image_based_timestamp(cv::Mat const &image, std::uint64_t timestamp);

   public:
	explicit ImageStreamRTSP(std::function<bool(const ImageData &)> &&image_mask = [](ImageData const &) { return true; });
	void operator()();

   private:
	void run(std::shared_ptr<ImageData const> const &data) final;
};