#pragma once

#include <gst/gst.h>
#include <gst/rtp/gstrtphdrext.h>

// G_BEGIN_DECLS

// struct _GstRTPHeaderExtensionRandomNumber {
//	GstRTPHeaderExtension parent;
//	guint64 random_number;
// };

typedef struct _GstRTPHeaderExtensionRandomNumber {
	GstRTPHeaderExtension parent;
	guint64 random_number;
} GstRTPHeaderExtensionRandomNumber;

typedef struct _GstRTPHeaderExtensionRandomNumberClass {
	GstRTPHeaderExtensionClass parent_class;
} GstRTPHeaderExtensionRandomNumberClass;

#define RANDOM_NUMBER_HDR_EXT_URI "random-number:1.0"

#define GST_TYPE_RTP_HEADER_EXTENSION_RANDOM_NUMBER (gst_rtp_header_extension_random_number_get_type())
#define GST_RTP_HEADER_EXTENSION_RANDOM_NUMBER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_RTP_HEADER_EXTENSION_RANDOM_NUMBER, GstRTPHeaderExtensionRandomNumber))
#define GST_RTP_HEADER_EXTENSION_RANDOM_NUMBER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_RTP_HEADER_EXTENSION_RANDOM_NUMBER, GstRTPHeaderExtensionRandomNumberClass))
#define GST_IS_RTP_HEADER_EXTENSION_RANDOM_NUMBER(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_RTP_HEADER_EXTENSION_RANDOM_NUMBER))
#define GST_IS_RTP_HEADER_EXTENSION_RANDOM_NUMBER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_RTP_HEADER_EXTENSION_RANDOM_NUMBER))

GType gst_rtp_header_extension_random_number_get_type(void);
GST_ELEMENT_REGISTER_DECLARE(rtp_header_extension_random_number)

//#define GST_TYPE_RTP_HEADER_EXTENSION_RANDOM_NUMBER (gst_rtp_header_extension_random_number_get_type())
// G_DECLARE_FINAL_TYPE(GstRTPHeaderExtensionRandomNumber, gst_rtp_header_extension_random_number, GST, RTP_HEADER_EXTENSION_RANDOM_NUMBER, GstRTPHeaderExtension);
//  GST_ELEMENT_REGISTER_DECLARE(rtphdrextrandomnumber);

// G_END_DECLS