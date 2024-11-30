#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iostream>

#include "rtp_header_extension_random_number.h"

G_DEFINE_TYPE(GstRTPHeaderExtensionRandomNumber, gst_rtp_header_extension_random_number, GST_TYPE_RTP_HEADER_EXTENSION);
GST_ELEMENT_REGISTER_DEFINE(rtp_header_extension_random_number, "rtp_header_extension_random_number", GST_RANK_NONE, GST_TYPE_RTP_HEADER_EXTENSION);

GST_DEBUG_CATEGORY_STATIC(gst_rtp_header_extension_random_number_debug);
#define GST_CAT_DEFAULT gst_rtp_header_extension_random_number_debug

#define DEFAULT_RANDOM_NUMBER 37

enum {
	PROP_0,
	PROP_RANDOM_NUMBER,
};

static GstRTPHeaderExtensionFlags gst_rtp_header_extension_random_number_get_supported_flags(GstRTPHeaderExtension *ext) {
	return static_cast<GstRTPHeaderExtensionFlags>(GST_RTP_HEADER_EXTENSION_ONE_BYTE | GST_RTP_HEADER_EXTENSION_TWO_BYTE);  // because sizeof(guint64) < 16
}
static gsize gst_rtp_header_extension_random_number_get_max_size(GstRTPHeaderExtension *ext, const GstBuffer *buffer) {
	GstRTPHeaderExtensionRandomNumber *self;  // = GST_RTP_HEADER_EXTENSION_RANDOM_NUMBER(ext);
	return sizeof(self->random_number);
}

static gssize gst_rtp_header_extension_random_number_write(GstRTPHeaderExtension *ext, const GstBuffer *input_meta, GstRTPHeaderExtensionFlags write_flags, GstBuffer *output, guint8 *data, gsize size) {
	GstRTPHeaderExtensionRandomNumber *self = GST_RTP_HEADER_EXTENSION_RANDOM_NUMBER(ext);

	g_return_val_if_fail(size >= gst_rtp_header_extension_random_number_get_max_size(ext, nullptr), -1);
	g_return_val_if_fail(write_flags & gst_rtp_header_extension_random_number_get_supported_flags(ext), -1);

	GST_OBJECT_LOCK(ext);

	std::cout << "writing random number " << self->random_number << std::endl;
	GST_LOG_OBJECT(self, "writing random number \'%lu\'", self->random_number);
	GST_WRITE_UINT64_BE(data, self->random_number);

	GST_OBJECT_UNLOCK(ext);

	return sizeof(self->random_number);
}

static gboolean gst_rtp_header_extension_random_number_read(GstRTPHeaderExtension *ext, GstRTPHeaderExtensionFlags read_flags, const guint8 *data, gsize size, GstBuffer *buffer) {
	GstRTPHeaderExtensionRandomNumber *self = GST_RTP_HEADER_EXTENSION_RANDOM_NUMBER(ext);
	gboolean notify = FALSE;

	if (!data || size == 0) return TRUE;

	if (read_flags & GST_RTP_HEADER_EXTENSION_ONE_BYTE && (size < 1 || size > 16)) {
		GST_ERROR_OBJECT(ext, "one-byte header extensions must be between 1 and 16 bytes inculusive");
		return FALSE;
	}

	GST_OBJECT_LOCK(self);
	self->random_number = GST_READ_UINT64_BE(data);
	GST_LOG_OBJECT(self, "reading random number \'%lu\'", self->random_number);
	std::cout << "reading random number " << self->random_number << std::endl;
	GST_OBJECT_UNLOCK(self);

	g_object_notify((GObject *)self, "random_number");

	return TRUE;
}

static void gst_rtp_header_extension_random_number_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec) {
	GstRTPHeaderExtensionRandomNumber *self = GST_RTP_HEADER_EXTENSION_RANDOM_NUMBER(object);

	switch (prop_id) {
		case PROP_RANDOM_NUMBER:
			GST_OBJECT_LOCK(self);
			g_value_set_uint64(value, self->random_number);
			GST_OBJECT_UNLOCK(self);
			break;
		default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec); break;
	}
}

static void gst_rtp_header_extension_random_number_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec) {
	GstRTPHeaderExtensionRandomNumber *self = GST_RTP_HEADER_EXTENSION_RANDOM_NUMBER(object);

	switch (prop_id) {
		case PROP_RANDOM_NUMBER:
			GST_OBJECT_LOCK(self);
			self->random_number = g_value_get_uint64(value);
			GST_OBJECT_UNLOCK(self);
			break;
		default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec); break;
	}
}

static void gst_rtp_header_extension_random_number_class_init(GstRTPHeaderExtensionRandomNumberClass *klass) {
	GstRTPHeaderExtensionClass *rtp_hdr_class = (GstRTPHeaderExtensionClass *)klass;
	GObjectClass *gobject_class = (GObjectClass *)klass;
	GstElementClass *gstelement_class = (GstElementClass *)klass;

	gobject_class->set_property = gst_rtp_header_extension_random_number_set_property;
	gobject_class->get_property = gst_rtp_header_extension_random_number_get_property;

	g_object_class_install_property(
	    gobject_class, PROP_RANDOM_NUMBER, g_param_spec_uint64("random-number", "Random Number", "A random number", 0, G_MAXUINT64, DEFAULT_RANDOM_NUMBER, static_cast<GParamFlags>(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

	rtp_hdr_class->get_supported_flags = gst_rtp_header_extension_random_number_get_supported_flags;
	rtp_hdr_class->get_max_size = gst_rtp_header_extension_random_number_get_max_size;
	rtp_hdr_class->write = gst_rtp_header_extension_random_number_write;
	rtp_hdr_class->read = gst_rtp_header_extension_random_number_read;

	gst_element_class_set_static_metadata(gstelement_class, "RTP Custom Header Extension Random Number", GST_RTP_HDREXT_ELEMENT_CLASS, "Extends RTP packets to add or retrieve a 64-bit random number", "Lukas Heyn <lukas.heyn@gmail.com>");
	gst_rtp_header_extension_class_set_uri(rtp_hdr_class, GST_RTP_HDREXT_BASE RANDOM_NUMBER_HDR_EXT_URI);
}

constexpr auto test = GST_RTP_HDREXT_BASE RANDOM_NUMBER_HDR_EXT_URI;

static void gst_rtp_header_extension_random_number_init(GstRTPHeaderExtensionRandomNumber *self) { self->random_number = 37ULL; }