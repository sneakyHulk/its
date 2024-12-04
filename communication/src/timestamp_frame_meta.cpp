#include "timestamp_frame_meta.h"

GST_DEBUG_CATEGORY_STATIC(gst_timestamp_frame_meta_debug);

GstTimestampFrameMeta *gst_buffer_add_timestamp_frame_meta(GstBuffer *buffer, GstCaps *reference, guint64 timestamp, guint64 frame) {
	GstTimestampFrameMeta *meta;

	g_return_val_if_fail(GST_IS_CAPS(reference), NULL);

	meta = (GstTimestampFrameMeta *)gst_buffer_add_meta(buffer, GST_TIMESTAMP_FRAME_META_INFO, NULL);

	if (!meta) return NULL;

	meta->reference = gst_caps_ref(reference);
	meta->timestamp = timestamp;
	meta->frame = frame;

	return meta;
}

GstTimestampFrameMeta *gst_buffer_get_timestamp_frame_meta(GstBuffer *buffer, GstCaps *reference) {
	gpointer state = NULL;
	GstMeta *meta;
	const GstMetaInfo *info = GST_TIMESTAMP_FRAME_META_INFO;

	while ((meta = gst_buffer_iterate_meta(buffer, &state))) {
		if (meta->info->api == info->api) {
			GstTimestampFrameMeta *rmeta = (GstTimestampFrameMeta *)meta;

			if (!reference) return rmeta;
			if (gst_caps_is_subset(rmeta->reference, reference)) return rmeta;
		}
	}
	return NULL;
}

static gboolean _gst_timestamp_frame_meta_transform(GstBuffer *dest, GstMeta *meta, GstBuffer *buffer, GQuark type, gpointer data) {
	GstTimestampFrameMeta *dmeta, *smeta;

	/* we copy over the reference timestamp meta, independent of transformation
	 * that happens. If it applied to the original buffer, it still applies to
	 * the new buffer as it refers to the time when the media was captured */
	smeta = (GstTimestampFrameMeta *)meta;
	dmeta = gst_buffer_add_timestamp_frame_meta(dest, smeta->reference, smeta->timestamp, smeta->frame);
	if (!dmeta) return FALSE;

	GST_CAT_DEBUG(gst_timestamp_frame_meta_debug, "copy random number metadata from buffer %p to %p", buffer, dest);

	return TRUE;
}

static void _gst_timestamp_frame_meta_free(GstTimestampFrameMeta *meta, GstBuffer *buffer) {}

static gboolean _gst_timestamp_frame_meta_init(GstTimestampFrameMeta *meta, gpointer params, GstBuffer *buffer) {
	static gsize _init;

	if (g_once_init_enter(&_init)) {
		GST_DEBUG_CATEGORY_INIT(gst_timestamp_frame_meta_debug, "timestamp_frame_meta", 0, "timestamp_frame_meta");
		g_once_init_leave(&_init, 1);
	}

	meta->reference = NULL;
	meta->timestamp = DEFAULT_TIMESTAMP;
	meta->frame = DEFAULT_FRAME;

	return TRUE;
}

GType gst_timestamp_frame_meta_api_get_type(void) {
	static GType type = 0;
	static const gchar *tags[] = {NULL};

	if (g_once_init_enter(&type)) {
		GType _type = gst_meta_api_type_register("GstTimestampFrameMetaAPI", tags);
		g_once_init_leave(&type, _type);
	}

	return type;
}

const GstMetaInfo *gst_timestamp_frame_meta_get_info(void) {
	static const GstMetaInfo *meta_info = NULL;

	if (g_once_init_enter((GstMetaInfo **)&meta_info)) {
		const GstMetaInfo *meta = gst_meta_register(gst_timestamp_frame_meta_api_get_type(), "GstTimestampFrameMeta", sizeof(GstTimestampFrameMeta), (GstMetaInitFunction)_gst_timestamp_frame_meta_init,
		    (GstMetaFreeFunction)_gst_timestamp_frame_meta_free, _gst_timestamp_frame_meta_transform);
		g_once_init_leave((GstMetaInfo **)&meta_info, (GstMetaInfo *)meta);
	}

	return meta_info;
}