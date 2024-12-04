#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>

#include <iostream>

static void configure_media(GstRTSPMediaFactory *factory, GstRTSPMedia *media, gpointer user_data) {
	// Create the pipeline
	GstElement *pipeline = gst_pipeline_new("rtsp-pipeline");
	if (!pipeline) {
		g_printerr("Failed to create pipeline\n");
		return;
	}

	// Create elements
	GstElement *src = gst_element_factory_make("videotestsrc", "source");
	GstElement *capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
	GstElement *queue = gst_element_factory_make("queue", "queue");
	GstElement *encoder = gst_element_factory_make("x264enc", "encoder");
	GstElement *payloader = gst_element_factory_make("rtph264pay", "payloader");

	if (!src || !capsfilter || !queue || !encoder || !payloader) {
		g_printerr("Failed to create elements\n");
		gst_object_unref(pipeline);
		return;
	}

	// Configure elements
	g_object_set(src, "pattern", 0, NULL);  // Pattern 0: Smpte
	GstCaps *caps = gst_caps_new_simple("video/x-raw", "width", G_TYPE_INT, 1280, "height", G_TYPE_INT, 720, "framerate", GST_TYPE_FRACTION, 30, 1, NULL);
	g_object_set(capsfilter, "caps", caps, NULL);
	gst_caps_unref(caps);

	g_object_set(encoder, "tune", 0x00000004 /* zerolatency */, NULL);

	// Add elements to the pipeline
	gst_bin_add_many(GST_BIN(pipeline), src, capsfilter, queue, encoder, payloader, NULL);

	// Link elements
	if (!gst_element_link_many(src, capsfilter, queue, encoder, payloader, NULL)) {
		g_printerr("Failed to link elements\n");
		gst_object_unref(pipeline);
		return;
	}

	// Attach the pipeline to the RTSP media
	gst_rtsp_media_take_pipeline(media, GST_PIPELINE(pipeline));

	gst_element_set_state(pipeline, GST_STATE_PLAYING);

	// The pipeline is now managed by the media and will be unreferenced when the media is destroyed
}

int main(int argc, char *argv[]) {
	gst_init(&argc, &argv);

	// Create an RTSP server
	GstRTSPServer *server = gst_rtsp_server_new();
	if (!server) {
		g_printerr("Failed to create RTSP server\n");
		return -1;
	}

	// Get the mount points
	GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(server);

	// Create a factory
	GstRTSPMediaFactory *factory = gst_rtsp_media_factory_new();
	g_signal_connect(factory, "media-configure", G_CALLBACK(configure_media), NULL);

	// Add the factory to the mount points
	gst_rtsp_mount_points_add_factory(mounts, "/test", factory);

	g_object_unref(mounts);

	// Attach the server to the main context
	GMainLoop *loop = g_main_loop_new(NULL, FALSE);
	if (gst_rtsp_server_attach(server, NULL) == 0) {
		g_printerr("Failed to attach RTSP server\n");
		return -1;
	}

	g_print("Stream ready at rtsp://127.0.0.1:8554/test\n");

	// Run the main loop
	g_main_loop_run(loop);

	// Clean up
	g_main_loop_unref(loop);
	g_object_unref(server);

	return 0;
}

/*
class CustomMediaFactory : public GstRTSPMediaFactory {
   protected:
    void on_media_configure(GstRTSPMedia *media) {
        // Create the custom pipeline for the RTSP stream.
        GstElement *pipeline = gst_pipeline_new("rtsp-pipeline");
        if (!pipeline) {
            g_printerr("Failed to create pipeline\n");
            return;
        }

        // Create elements.
        GstElement *src = gst_element_factory_make("videotestsrc", "source");
        GstElement *capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
        GstElement *queue = gst_element_factory_make("queue", "queue");
        GstElement *encoder = gst_element_factory_make("x264enc", "encoder");
        GstElement *payloader = gst_element_factory_make("rtph264pay", "payloader");

        if (!src || !capsfilter || !queue || !encoder || !payloader) {
            g_printerr("Failed to create elements\n");
            gst_object_unref(pipeline);
            return;
        }

        // Configure elements.
        g_object_set(src, "pattern", 0, NULL);  // Pattern 0: Smpte
        GstCaps *caps = gst_caps_new_simple("video/x-raw", "width", G_TYPE_INT, 1280, "height", G_TYPE_INT, 720, "framerate", GST_TYPE_FRACTION, 30, 1, NULL);
        g_object_set(capsfilter, "caps", caps, NULL);
        gst_caps_unref(caps);

        g_object_set(encoder, "tune", 0x00000004, NULL);

        // Add elements to the pipeline.
        gst_bin_add_many(GST_BIN(pipeline), src, capsfilter, queue, encoder, payloader, NULL);

        // Link elements.
        if (!gst_element_link_many(src, capsfilter, queue, encoder, payloader, NULL)) {
            g_printerr("Failed to link elements\n");
            gst_object_unref(pipeline);
            return;
        }

        // Attach the pipeline to the RTSP media.
        gst_rtsp_media_take_pipeline(media, reinterpret_cast<GstPipeline *>(pipeline));

        gst_element_set_state(pipeline, GST_STATE_PLAYING);

        // The pipeline is now managed by the media and will be unreferenced when the media is destroyed.
    }

    static void media_configure_callback(GstRTSPMediaFactory *factory, GstRTSPMedia *media, gpointer user_data) { static_cast<CustomMediaFactory *>(user_data)->on_media_configure(media); }

   public:
    CustomMediaFactory() : GstRTSPMediaFactory(*gst_rtsp_media_factory_new()) {
        gst_rtsp_media_factory_set_media_gtype(GST_RTSP_MEDIA_FACTORY(this), GST_TYPE_RTSP_MEDIA);
        g_signal_connect(GST_RTSP_MEDIA_FACTORY(this), "media-configure", G_CALLBACK(media_configure_callback), this);
    }
};

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);

    // Create an RTSP server.
    GstRTSPServer *server = gst_rtsp_server_new();
    if (!server) {
        g_printerr("Failed to create RTSP server\n");
        return -1;
    }

    // Get the mount points and attach a factory.
    GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(server);
    CustomMediaFactory *factory = new CustomMediaFactory();

    gst_rtsp_mount_points_add_factory(mounts, "/test", GST_RTSP_MEDIA_FACTORY(factory));

    g_object_unref(mounts);

    // Attach the server to the main context.
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    if (gst_rtsp_server_attach(server, NULL) == 0) {
        g_printerr("Failed to attach RTSP server\n");
        return -1;
    }

    g_print("Stream ready at rtsp://127.0.0.1:8554/test\n");

    // Run the main loop.
    g_main_loop_run(loop);

    // Clean up.
    g_main_loop_unref(loop);
    g_object_unref(server);

    return 0;
}*/
