#include "random_number_meta.h"

GST_DEBUG_CATEGORY_STATIC(gst_random_number_meta_debug);

GstRandomNumberMeta *gst_buffer_add_random_number_meta(GstBuffer *buffer, GstCaps *reference, guint64 random_number) {
	GstRandomNumberMeta *meta;

	g_return_val_if_fail(GST_IS_CAPS(reference), NULL);

	meta = (GstRandomNumberMeta *)gst_buffer_add_meta(buffer, GST_RANDOM_NUMBER_META_INFO, NULL);

	if (!meta) return NULL;

	meta->reference = gst_caps_ref(reference);
	meta->random_number = random_number;

	return meta;
}

GstRandomNumberMeta *gst_buffer_get_random_number_meta(GstBuffer *buffer, GstCaps *reference) {
	gpointer state = NULL;
	GstMeta *meta;
	const GstMetaInfo *info = GST_RANDOM_NUMBER_META_INFO;

	while ((meta = gst_buffer_iterate_meta(buffer, &state))) {
		if (meta->info->api == info->api) {
			GstRandomNumberMeta *rmeta = (GstRandomNumberMeta *)meta;

			if (!reference) return rmeta;
			if (gst_caps_is_subset(rmeta->reference, reference)) return rmeta;
		}
	}
	return NULL;
}

static gboolean _gst_random_number_meta_transform(GstBuffer *dest, GstMeta *meta, GstBuffer *buffer, GQuark type, gpointer data) {
	GstRandomNumberMeta *dmeta, *smeta;

	/* we copy over the reference timestamp meta, independent of transformation
	 * that happens. If it applied to the original buffer, it still applies to
	 * the new buffer as it refers to the time when the media was captured */
	smeta = (GstRandomNumberMeta *)meta;
	dmeta = gst_buffer_add_random_number_meta(dest, smeta->reference, smeta->random_number);
	if (!dmeta) return FALSE;

	GST_CAT_DEBUG(gst_random_number_meta_debug, "copy random number metadata from buffer %p to %p", buffer, dest);

	return TRUE;
}

static void _gst_random_number_meta_free(GstRandomNumberMeta *meta, GstBuffer *buffer) {
	if (meta->reference) gst_caps_unref(meta->reference);
}

static gboolean _gst_random_number_meta_init(GstRandomNumberMeta *meta, gpointer params, GstBuffer *buffer) {
	static gsize _init;

	if (g_once_init_enter(&_init)) {
		GST_DEBUG_CATEGORY_INIT(gst_random_number_meta_debug, "random_number_meta", 0, "random_number_meta");
		g_once_init_leave(&_init, 1);
	}

	meta->reference = NULL;
	meta->random_number = 37ULL;

	return TRUE;
}

GType gst_random_number_meta_api_get_type(void) {
	static GType type = 0;
	static const gchar *tags[] = {NULL};

	if (g_once_init_enter(&type)) {
		GType _type = gst_meta_api_type_register("GstRandomNumberMetaAPI", tags);
		g_once_init_leave(&type, _type);
	}

	return type;
}

const GstMetaInfo *gst_random_number_meta_get_info(void) {
	static const GstMetaInfo *meta_info = NULL;

	if (g_once_init_enter((GstMetaInfo **)&meta_info)) {
		const GstMetaInfo *meta = gst_meta_register(
		    gst_random_number_meta_api_get_type(), "GstRandomNumberMeta", sizeof(GstRandomNumberMeta), (GstMetaInitFunction)_gst_random_number_meta_init, (GstMetaFreeFunction)_gst_random_number_meta_free, _gst_random_number_meta_transform);
		g_once_init_leave((GstMetaInfo **)&meta_info, (GstMetaInfo *)meta);
	}

	return meta_info;
}