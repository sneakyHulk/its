#pragma once

#include <gst/app/gstappsrc.h>
#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>

#include <vector>

#include "ImageData.h"
#include "node.h"

struct StreamContext {
	GstClockTime timestamp;
	std::shared_ptr<ImageData const> *image;
};

class ImageStreamRTSP : public OutputPtrNode<ImageData> {
	static constexpr bool use_jpeg = false;
	static const char *port;

	static GstRTSPServer *_server;
	static GstRTSPMountPoints *_mounts;
	static GMainLoop *_loop;

	static std::vector<ImageStreamRTSP *> all_streams;

	std::shared_ptr<ImageData const> _image_data = nullptr;
	GstRTSPMediaFactory *_factory;

   private:
	static void need_data(GstElement *appsrc, guint unused, StreamContext *context);
	static void media_configure(GstRTSPMediaFactory *factory, GstRTSPMedia *media, ImageStreamRTSP *current_class);

   public:
	ImageStreamRTSP();
	[[noreturn]] static void run_loop();

   private:
	void output_function(std::shared_ptr<ImageData const> const &data) final;
};