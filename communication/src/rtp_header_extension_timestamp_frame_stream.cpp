#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cstring>
#include <iostream>

#include "rtp_header_extension_timestamp_frame_stream.h"
#include "timestamp_frame_meta.h"

G_DEFINE_TYPE(GstRTPHeaderExtensionTimestampFrameStream, gst_rtp_header_extension_timestamp_frame_stream, GST_TYPE_RTP_HEADER_EXTENSION);
GST_ELEMENT_REGISTER_DEFINE(rtp_header_extension_timestamp_frame_stream, "rtp_header_extension_timestamp_frame_stream", GST_RANK_NONE, GST_TYPE_RTP_HEADER_EXTENSION);

GST_DEBUG_CATEGORY_STATIC(gst_rtp_header_extension_timestamp_frame_stream_debug);
#define GST_CAT_DEFAULT gst_rtp_header_extension_timestamp_frame_stream_debug

enum {
	PROP_0,
};

static GstStaticCaps timestamp_frame_stream_caps = GST_STATIC_CAPS("timestamp-frame/x-stream");

static GstRTPHeaderExtensionFlags gst_rtp_header_extension_timestamp_frame_stream_get_supported_flags(GstRTPHeaderExtension *ext) {
	return static_cast<GstRTPHeaderExtensionFlags>(GST_RTP_HEADER_EXTENSION_ONE_BYTE | GST_RTP_HEADER_EXTENSION_TWO_BYTE);  // because sizeof(guint64) <= 16
}
static gsize gst_rtp_header_extension_timestamp_frame_stream_get_max_size(GstRTPHeaderExtension *ext, const GstBuffer *buffer) {
	GstRTPHeaderExtensionTimestampFrameStream *self = GST_RTP_HEADER_EXTENSION_TIMESTAMP_FRAME_STREAM(ext);
	return sizeof(guint64) + sizeof(guint64);
}

#define GST_LOG_OBJECT_WARN(obj, ...) GST_CAT_LEVEL_LOG(GST_CAT_DEFAULT, GST_LEVEL_WARNING, obj, __VA_ARGS__)

static gssize gst_rtp_header_extension_timestamp_frame_stream_write(GstRTPHeaderExtension *ext, const GstBuffer *input_meta, GstRTPHeaderExtensionFlags write_flags, GstBuffer *output, guint8 *data, gsize size) {
	GstRTPHeaderExtensionTimestampFrameStream *self = GST_RTP_HEADER_EXTENSION_TIMESTAMP_FRAME_STREAM(ext);

	g_return_val_if_fail(size >= gst_rtp_header_extension_timestamp_frame_stream_get_max_size(ext, nullptr), -1);
	g_return_val_if_fail(write_flags & gst_rtp_header_extension_timestamp_frame_stream_get_supported_flags(ext), -1);

	GstCaps *caps = gst_static_caps_get(&timestamp_frame_stream_caps);
	GstTimestampFrameMeta *meta = gst_buffer_get_timestamp_frame_meta((GstBuffer *)input_meta, caps);

	if (meta) {
		GST_LOG_OBJECT(self, "writing timestamp \'%lu\' and frame \'%lu\'", meta->timestamp, meta->frame);
		GST_WRITE_UINT64_BE(data, meta->timestamp);
		GST_WRITE_UINT64_BE(data + sizeof(guint64), meta->frame);
	} else {
		GST_LOG_OBJECT_WARN(self, "no metadata found, writing timestamp \'%lu\' and frame \'%lu\'", DEFAULT_TIMESTAMP, DEFAULT_FRAME);
		GST_WRITE_UINT64_BE(data, 0);
		GST_WRITE_UINT64_BE(data + sizeof(guint64), 0);
	}

	gst_caps_unref(caps);

	return sizeof(guint64);
}

static gboolean gst_rtp_header_extension_timestamp_frame_stream_read(GstRTPHeaderExtension *ext, GstRTPHeaderExtensionFlags read_flags, const guint8 *data, gsize size, GstBuffer *buffer) {
	GstRTPHeaderExtensionTimestampFrameStream *self = GST_RTP_HEADER_EXTENSION_TIMESTAMP_FRAME_STREAM(ext);

	if (size < sizeof(guint64)) return FALSE;

	GstCaps *caps = gst_static_caps_get(&timestamp_frame_stream_caps);

	auto timestamp = GST_READ_UINT64_BE(data);
	auto frame = GST_READ_UINT64_BE(data + sizeof(guint64));
	gst_buffer_add_timestamp_frame_meta(buffer, caps, timestamp, frame);

	gst_caps_unref(caps);

	GST_LOG_OBJECT(self, "reading timestamp \'%lu\' and frame \'%lu\'", timestamp, frame);

	return TRUE;
}

static void gst_rtp_header_extension_timestamp_frame_stream_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec) {
	GstRTPHeaderExtensionTimestampFrameStream *self = GST_RTP_HEADER_EXTENSION_TIMESTAMP_FRAME_STREAM(object);

	switch (prop_id) {
		default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec); break;
	}
}

static void gst_rtp_header_extension_timestamp_frame_stream_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec) {
	GstRTPHeaderExtensionTimestampFrameStream *self = GST_RTP_HEADER_EXTENSION_TIMESTAMP_FRAME_STREAM(object);

	switch (prop_id) {
		default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec); break;
	}
}

static void gst_rtp_header_extension_timestamp_frame_stream_class_init(GstRTPHeaderExtensionTimestampFrameStreamClass *klass) {
	GstRTPHeaderExtensionClass *rtp_hdr_class = (GstRTPHeaderExtensionClass *)klass;
	GObjectClass *gobject_class = (GObjectClass *)klass;
	GstElementClass *gstelement_class = (GstElementClass *)klass;

	gobject_class->set_property = gst_rtp_header_extension_timestamp_frame_stream_set_property;
	gobject_class->get_property = gst_rtp_header_extension_timestamp_frame_stream_get_property;

	rtp_hdr_class->get_supported_flags = gst_rtp_header_extension_timestamp_frame_stream_get_supported_flags;
	rtp_hdr_class->get_max_size = gst_rtp_header_extension_timestamp_frame_stream_get_max_size;
	rtp_hdr_class->write = gst_rtp_header_extension_timestamp_frame_stream_write;
	rtp_hdr_class->read = gst_rtp_header_extension_timestamp_frame_stream_read;

	gst_element_class_set_static_metadata(gstelement_class, "RTP Custom Header Extension Timestamp and Frame Number Stream", GST_RTP_HDREXT_ELEMENT_CLASS, "Extends RTP packets to add or retrieve a 64-bit timestamp and frame number stream",
	    "Lukas Heyn <lukas.heyn@gmail.com>");
	gst_rtp_header_extension_class_set_uri(rtp_hdr_class, GST_RTP_HDREXT_BASE TIMESTAMP_FRAME_STREAM_HDR_EXT_URI);
}

static void gst_rtp_header_extension_timestamp_frame_stream_init(GstRTPHeaderExtensionTimestampFrameStream *self) {}