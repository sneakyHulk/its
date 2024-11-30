#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cstring>
#include <iostream>

#include "random_number_meta.h"
#include "rtp_header_extension_random_number_stream.h"

G_DEFINE_TYPE(GstRTPHeaderExtensionRandomNumberStream, gst_rtp_header_extension_random_number_stream, GST_TYPE_RTP_HEADER_EXTENSION);
GST_ELEMENT_REGISTER_DEFINE(rtp_header_extension_random_number_stream, "rtp_header_extension_random_number_stream", GST_RANK_NONE, GST_TYPE_RTP_HEADER_EXTENSION);

GST_DEBUG_CATEGORY_STATIC(gst_rtp_header_extension_random_number_stream_debug);
#define GST_CAT_DEFAULT gst_rtp_header_extension_random_number_stream_debug

#define DEFAULT_RANDOM_NUMBER 37

enum {
	PROP_0,
};

static GstStaticCaps random_number_stream_caps = GST_STATIC_CAPS("random-number/x-stream");

static GstRTPHeaderExtensionFlags gst_rtp_header_extension_random_number_stream_get_supported_flags(GstRTPHeaderExtension *ext) {
	return static_cast<GstRTPHeaderExtensionFlags>(GST_RTP_HEADER_EXTENSION_ONE_BYTE | GST_RTP_HEADER_EXTENSION_TWO_BYTE);  // because sizeof(guint64) <= 16
}
static gsize gst_rtp_header_extension_random_number_stream_get_max_size(GstRTPHeaderExtension *ext, const GstBuffer *buffer) {
	GstRTPHeaderExtensionRandomNumberStream *self = GST_RTP_HEADER_EXTENSION_RANDOM_NUMBER_STREAM(ext);
	return sizeof(guint64);
}

static gssize gst_rtp_header_extension_random_number_stream_write(GstRTPHeaderExtension *ext, const GstBuffer *input_meta, GstRTPHeaderExtensionFlags write_flags, GstBuffer *output, guint8 *data, gsize size) {
	GstRTPHeaderExtensionRandomNumberStream *self = GST_RTP_HEADER_EXTENSION_RANDOM_NUMBER_STREAM(ext);

	g_return_val_if_fail(size >= gst_rtp_header_extension_random_number_stream_get_max_size(ext, nullptr), -1);
	g_return_val_if_fail(write_flags & gst_rtp_header_extension_random_number_stream_get_supported_flags(ext), -1);

	GstCaps *caps = gst_static_caps_get(&random_number_stream_caps);
	GstRandomNumberMeta *meta = gst_buffer_get_random_number_meta((GstBuffer *)input_meta, caps);

	if (meta) {
		std::cout << "writing random number " << meta->random_number << std::endl;
		GST_LOG_OBJECT(self, "writing random number \'%lu\'", meta->random_number);
		GST_WRITE_UINT64_BE(data, meta->random_number);
	} else {
		std::cout << "no metadata found, writing random number " << 37UL << std::endl;
		GST_LOG_OBJECT(self, "no metadata found, writing random number \'%lu\'", 37UL);
		std::memset(data, 37ULL, sizeof(guint64));
	}

	gst_caps_unref(caps);

	return sizeof(guint64);
}

static gboolean gst_rtp_header_extension_random_number_stream_read(GstRTPHeaderExtension *ext, GstRTPHeaderExtensionFlags read_flags, const guint8 *data, gsize size, GstBuffer *buffer) {
	GstRTPHeaderExtensionRandomNumberStream *self = GST_RTP_HEADER_EXTENSION_RANDOM_NUMBER_STREAM(ext);

	if (size < sizeof(guint64)) return FALSE;

	GstCaps *caps = gst_static_caps_get(&random_number_stream_caps);

	auto random_number = GST_READ_UINT64_BE(data);
	gst_buffer_add_random_number_meta(buffer, caps, random_number);

	gst_caps_unref(caps);

	std::cout << "reading random number " << random_number << std::endl;

	return TRUE;
}

static void gst_rtp_header_extension_random_number_stream_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec) {
	GstRTPHeaderExtensionRandomNumberStream *self = GST_RTP_HEADER_EXTENSION_RANDOM_NUMBER_STREAM(object);

	switch (prop_id) {
		default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec); break;
	}
}

static void gst_rtp_header_extension_random_number_stream_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec) {
	GstRTPHeaderExtensionRandomNumberStream *self = GST_RTP_HEADER_EXTENSION_RANDOM_NUMBER_STREAM(object);

	switch (prop_id) {
		default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec); break;
	}
}

static void gst_rtp_header_extension_random_number_stream_class_init(GstRTPHeaderExtensionRandomNumberStreamClass *klass) {
	GstRTPHeaderExtensionClass *rtp_hdr_class = (GstRTPHeaderExtensionClass *)klass;
	GObjectClass *gobject_class = (GObjectClass *)klass;
	GstElementClass *gstelement_class = (GstElementClass *)klass;

	gobject_class->set_property = gst_rtp_header_extension_random_number_stream_set_property;
	gobject_class->get_property = gst_rtp_header_extension_random_number_stream_get_property;

	rtp_hdr_class->get_supported_flags = gst_rtp_header_extension_random_number_stream_get_supported_flags;
	rtp_hdr_class->get_max_size = gst_rtp_header_extension_random_number_stream_get_max_size;
	rtp_hdr_class->write = gst_rtp_header_extension_random_number_stream_write;
	rtp_hdr_class->read = gst_rtp_header_extension_random_number_stream_read;

	gst_element_class_set_static_metadata(
	    gstelement_class, "RTP Custom Header Extension Random Number Stream", GST_RTP_HDREXT_ELEMENT_CLASS, "Extends RTP packets to add or retrieve a 64-bit random number stream", "Lukas Heyn <lukas.heyn@gmail.com>");
	gst_rtp_header_extension_class_set_uri(rtp_hdr_class, GST_RTP_HDREXT_BASE RANDOM_NUMBER_STREAM_HDR_EXT_URI);
}

static void gst_rtp_header_extension_random_number_stream_init(GstRTPHeaderExtensionRandomNumberStream *self) {}