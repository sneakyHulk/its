#pragma once
#include <gst/gst.h>

typedef struct _GstTimestampFrameMeta GstTimestampFrameMeta;

struct _GstTimestampFrameMeta {
	GstMeta parent;

	/*< public >*/
	GstCaps *reference;
	guint64 timestamp;
	guint64 frame;
};

#define DEFAULT_TIMESTAMP 0UL
#define DEFAULT_FRAME 0UL

GType gst_timestamp_frame_meta_api_get_type(void);
#define GST_TIMESTAMP_FRAME_META_API_TYPE (gst_timestamp_frame_meta_api_get_type())

const GstMetaInfo *gst_timestamp_frame_meta_get_info(void);
#define GST_TIMESTAMP_FRAME_META_INFO (gst_timestamp_frame_meta_get_info())

GstTimestampFrameMeta *gst_buffer_add_timestamp_frame_meta(GstBuffer *buffer, GstCaps *reference, guint64 timestamp, guint64 frame);
GstTimestampFrameMeta *gst_buffer_get_timestamp_frame_meta(GstBuffer *buffer, GstCaps *reference);