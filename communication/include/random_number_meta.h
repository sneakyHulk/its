#pragma once
#include <gst/gst.h>

typedef struct _GstRandomNumberMeta GstRandomNumberMeta;

struct _GstRandomNumberMeta {
	GstMeta parent;

	/*< public >*/
	GstCaps *reference;
	guint64 random_number;
};

GType gst_random_number_meta_api_get_type(void);
#define GST_RANDOM_NUMBER_META_API_TYPE (gst_random_number_meta_api_get_type())

const GstMetaInfo *gst_random_number_meta_get_info(void);
#define GST_RANDOM_NUMBER_META_INFO (gst_random_number_meta_get_info())

GstRandomNumberMeta *gst_buffer_add_random_number_meta(GstBuffer *buffer, GstCaps *reference, guint64 random_number);

GstRandomNumberMeta *gst_buffer_get_random_number_meta(GstBuffer *buffer, GstCaps *reference);