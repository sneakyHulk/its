#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>

#include "rtp_header_extension_random_number.h"

static gboolean plugin_init_rtp_header_extension_random_number_static(GstPlugin *plugin) {
	GST_DEBUG_CATEGORY_STATIC(gst_rtp_header_extension_random_number_debug);
	GST_DEBUG_CATEGORY_INIT(gst_rtp_header_extension_random_number_debug, "rtp_header_extension_random_number", 0, "Random number RTP Header Extension");
	return gst_element_register(plugin, "rtp_header_extension_random_number", GST_RANK_NONE, GST_TYPE_RTP_HEADER_EXTENSION_RANDOM_NUMBER);
}

// static gboolean plugin_init_rtp_header_extension_random_number(GstPlugin *plugin) { return GST_ELEMENT_REGISTER(rtp_header_extension_random_number, plugin); }

#ifndef PACKAGE_RANDOM_NUMBER
#define PACKAGE_RANDOM_NUMBER "rtp_header_extension_random_number"
#endif

#ifndef VERSION_RANDOM_NUMBER
#define VERSION_RANDOM_NUMBER "1.0.0.0"
#endif

#ifndef GST_PACKAGE_NAME_RANDOM_NUMBER
#define GST_PACKAGE_NAME_RANDOM_NUMBER "GStreamer"
#endif

#ifndef GST_PACKAGE_ORIGIN_RANDOM_NUMBER
#define GST_PACKAGE_ORIGIN_RANDOM_NUMBER "http://heyn.dev/"
#endif

// GST_PLUGIN_DEFINE(GST_VERSION_MAJOR, GST_VERSION_MINOR, rtp_header_extension_random_number, "Random number RTP Header Extension", plugin_init_rtp_header_extension_random_number, VERSION, "LGPL", GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN);
void gst_rtp_header_extension_random_number_register_static() {
	gst_plugin_register_static(GST_VERSION_MAJOR, GST_VERSION_MINOR, "rtp_header_extension_random_number", "Random number RTP Header Extension", plugin_init_rtp_header_extension_random_number_static, VERSION_RANDOM_NUMBER, "LGPL",
	    "rtp_header_extension_random_number", GST_PACKAGE_NAME_RANDOM_NUMBER, GST_PACKAGE_ORIGIN_RANDOM_NUMBER);
}