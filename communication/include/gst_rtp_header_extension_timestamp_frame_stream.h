#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>

#include "rtp_header_extension_timestamp_frame_stream.h"

static gboolean plugin_init_rtp_header_extension_timestamp_frame_stream_static(GstPlugin *plugin) {
	GST_DEBUG_CATEGORY_STATIC(gst_rtp_header_extension_timestamp_frame_stream_debug);
	GST_DEBUG_CATEGORY_INIT(gst_rtp_header_extension_timestamp_frame_stream_debug, "rtp_header_extension_timestamp_frame_stream", 0, "Random number stream RTP Header Extension");
	return gst_element_register(plugin, "rtp_header_extension_timestamp_frame_stream", GST_RANK_NONE, GST_TYPE_RTP_HEADER_EXTENSION_TIMESTAMP_FRAME_STREAM);
}

// static gboolean plugin_init_rtp_header_extension_timestamp_frame_stream(GstPlugin *plugin) { return GST_ELEMENT_REGISTER(rtp_header_extension_timestamp_frame_stream, plugin); }

#ifndef PACKAGE_TIMESTAMP_FRAME_STREAM
#define PACKAGE_TIMESTAMP_FRAME_STREAM "rtp_header_extension_timestamp_frame_stream"
#endif

#ifndef VERSION_TIMESTAMP_FRAME_STREAM
#define VERSION_TIMESTAMP_FRAME_STREAM "1.0.0.0"
#endif

#ifndef GST_PACKAGE_NAME_TIMESTAMP_FRAME_STREAM
#define GST_PACKAGE_NAME_TIMESTAMP_FRAME_STREAM "GStreamer"
#endif

#ifndef GST_PACKAGE_ORIGIN_TIMESTAMP_FRAME_STREAM
#define GST_PACKAGE_ORIGIN_TIMESTAMP_FRAME_STREAM "http://heyn.dev/"
#endif

// GST_PLUGIN_DEFINE(GST_VERSION_MAJOR, GST_VERSION_MINOR, rtp_header_extension_timestamp_frame_stream, "Timestamp and frame number stream RTP Header Extension", plugin_init_rtp_header_extension_timestamp_frame_stream, VERSION, "LGPL", GST_PACKAGE_NAME,
//     GST_PACKAGE_ORIGIN);
void gst_rtp_header_extension_timestamp_frame_stream_register_static() {
	gst_plugin_register_static(GST_VERSION_MAJOR, GST_VERSION_MINOR, "rtp_header_extension_timestamp_frame_stream", "Timestamp and frame number stream RTP Header Extension", plugin_init_rtp_header_extension_timestamp_frame_stream_static, VERSION_TIMESTAMP_FRAME_STREAM, "LGPL",
	    "rtp_header_extension_timestamp_frame_stream", GST_PACKAGE_NAME_TIMESTAMP_FRAME_STREAM, GST_PACKAGE_ORIGIN_TIMESTAMP_FRAME_STREAM);
}