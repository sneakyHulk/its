#include <gst/app/gstappsrc.h>
#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <gst/rtsp/rtsp.h>

#include <chrono>
#include <cstring>
#include <iostream>
#include <random>
#include <thread>
#include <utility>

using namespace std::chrono_literals;

/*
int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);

    GstRTSPServer *server;
    GstRTSPMountPoints *mounts;
    GstRTSPMediaFactory *factory;

    GstRTSPUrl *url;
    GstElement *element;
    GstRTSPMedia *media;
    GstElement *pipeline;
    GstElement *appsrc;
    std::thread loop;

    guint64 timestamp = 0;
    int width = 320;
    int height = 240;
    int fps = 30;
    int size = width * height * 3;  // RGB
    guint8 *data = new guint8[size];
    GstBuffer *buffer;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(1, 255);

    factory = gst_rtsp_media_factory_new();
    gst_rtsp_media_factory_set_shared(factory, TRUE);

    if (!gst_rtsp_media_factory_is_shared(factory)) goto not_shared;

    if (gst_rtsp_url_parse("rtsp://127.0.0.1:8554/test", &url) != GST_RTSP_OK) goto url_not_ok;

    gst_rtsp_media_factory_set_launch(factory, "( appsrc name=mysrc is-live=true format=3 ! videoconvert ! x264enc speed-preset=ultrafast tune=zerolatency ! rtph264pay name=pay0 pt=96 )");

    server = gst_rtsp_server_new();
    mounts = gst_rtsp_server_get_mount_points(server);
    gst_rtsp_server_set_service(server, "8554");
    gst_rtsp_mount_points_add_factory(mounts, "/test", factory);

    // element = gst_rtsp_media_factory_create_element(factory, url);
    // if (!GST_IS_BIN(element)) goto element_not_created;

    media = gst_rtsp_media_factory_construct(factory, url);
    pipeline = gst_rtsp_media_get_element(media);

    appsrc = gst_bin_get_by_name_recurse_up(GST_BIN(pipeline), "mysrc");
    gst_util_set_object_arg(G_OBJECT(appsrc), "format", "time");

    // Start the server
    if (!gst_rtsp_server_attach(server, NULL)) {
        std::cerr << "Failed to attach the RTSP server" << std::endl;
        return -1;
    }

    if (GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING); ret == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "Failed to start the pipeline" << std::endl;
        gst_object_unref(pipeline);
        return -1;
    }

    loop = std::thread([]() {
        for (;;) {
            g_main_context_iteration(NULL, false);
        }
    });

    while (true) {
        memset(data, dist(gen), size);  // Fill with black

        // Create a new buffer for each frame
        buffer = gst_buffer_new_allocate(NULL, size, NULL);

        // Fill the buffer with data
        GstMapInfo map;
        gst_buffer_map(buffer, &map, GST_MAP_WRITE);
        std::memcpy(map.data, data, size);
        gst_buffer_unmap(buffer, &map);

        // Set the timestamp
        GST_BUFFER_PTS(buffer) = timestamp;
        GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, fps);
        timestamp += GST_BUFFER_DURATION(buffer);

        // Push buffer to appsrc
        if (GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(appsrc), buffer); ret != GST_FLOW_OK) {
            std::cerr << "Error pushing buffer to appsrc!" << std::endl;
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    //
    // if (!GST_IS_BIN(element)) throw;
    // if (GST_IS_PIPELINE(element)) throw;
    // gst_object_unref(element);
    // gst_rtsp_url_free(url);

    return 0;

element_not_created:
    gst_object_unref(element);
    std::cerr << "ERROR: gst_rtsp_media_factory_create_element" << std::endl;
url_not_ok:
    gst_rtsp_url_free(url);
    std::cerr << "ERROR: gst_rtsp_url_parse" << std::endl;
not_shared:
    g_object_unref(factory);
    std::cerr << "ERROR: gst_rtsp_media_factory_set_shared" << std::endl;
}*/

// GstElement *appsrc = nullptr;
static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_int_distribution<int> dist(1, 255);

static int framecount = 0;
GstBuffer *buffer;
GstFlowReturn ret;
guint64 timestamp = 0;

// Set up a simple test pattern (e.g., a black frame)
int width = 320;
int height = 240;
int fps = 30;
int size = width * height * 3;  // RGB
guint8 *data = new guint8[size];

// This function generates raw video frames and pushes them into appsrc
void push_data_to_appsrc(GstElement *appsrc2) { delete[] data; }

int main(int argc, char *argv[]) {
	gst_init(&argc, &argv);

	// Create an RTSP server
	GstRTSPServer *server = gst_rtsp_server_new();
	GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(server);
	gst_rtsp_server_set_service(server, "8554");

	// Create a factory for the media stream
	GstRTSPMediaFactory *factory = gst_rtsp_media_factory_new();

	// The pipeline description using appsrc
	gst_rtsp_media_factory_set_launch(factory, "( appsrc name=mysrc is-live=true format=3 emit-signals=false ! videoconvert ! video/x-raw,format=I420 ! x264enc speed-preset=ultrafast tune=zerolatency ! rtph264pay name=pay0 pt=96 )");
	gst_rtsp_media_factory_set_shared(factory, true);

	// Attach the factory to the /test endpoint
	gst_rtsp_mount_points_add_factory(mounts, "/test", factory);
	g_object_unref(mounts);

	// Start the server
	if (!gst_rtsp_server_attach(server, NULL)) {
		std::cerr << "Failed to attach the RTSP server" << std::endl;
		return -1;
	}

	GstElement **appsrc = new GstElement *(nullptr);
	// Add a callback to set up appsrc
	g_signal_connect(factory, "media-configure", G_CALLBACK(+[](GstRTSPMediaFactory *, GstRTSPMedia *media, GstElement **appsrc) {
		GstElement *pipeline = gst_rtsp_media_get_element(media);
		*appsrc = gst_bin_get_by_name_recurse_up(GST_BIN(pipeline), "mysrc");
		gst_util_set_object_arg(G_OBJECT(*appsrc), "format", "time");

		std::cout << "media" << std::endl;

		GstCaps *caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "BGR", "width", G_TYPE_INT, 320, "height", G_TYPE_INT, 240, "framerate", GST_TYPE_FRACTION, 30, 1, NULL);
		gst_app_src_set_caps(GST_APP_SRC(*appsrc), caps);
		gst_caps_unref(caps);

		gst_element_set_state(pipeline, GST_STATE_PLAYING);

		// Start pushing data to appsrc in a separate thread
		// std::thread(push_data_to_appsrc, appsrc).detach();
		// g_object_unref(appsrc);
	}),
	    appsrc);

	std::cout << "RTSP server is running at rtsp://127.0.0.1:8554/test" << std::endl;

	std::thread loop([]() {
		for (;;) {
			g_main_context_iteration(NULL, false);
		}
	});

	guint64 frameCount = 0;
	// Run the main loop
	for (;;) {
		if (!*appsrc) continue;

		GstState state;
		GstStateChangeReturn ret = gst_element_get_state(*appsrc, &state, nullptr, GST_CLOCK_TIME_NONE);

		if (ret != GST_STATE_CHANGE_SUCCESS || state != GST_STATE_PLAYING) {
			std::cerr << "Pipeline is not in PLAYING state" << std::endl;
			continue;
		}

		memset(data, dist(gen), size);  // Fill with black

		// Create a new buffer for each frame
		buffer = gst_buffer_new_allocate(NULL, size, NULL);

		// Fill the buffer with data
		GstMapInfo map;
		gst_buffer_map(buffer, &map, GST_MAP_WRITE);
		memcpy(map.data, data, size);
		gst_buffer_unmap(buffer, &map);

		// Set the timestamp
		GST_BUFFER_PTS(buffer) = timestamp;
		GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, fps);
		timestamp += GST_BUFFER_DURATION(buffer);

		GstStructure *metadata = gst_structure_new("frame-metadata", "frame-count", G_TYPE_UINT64, frameCount++, "timestamp", G_TYPE_UINT64, timestamp, NULL);
		GstMeta *meta = gst_buffer_add_meta(buffer, gst_meta_get_info("GstStructureMeta"), metadata);

		// Push buffer to appsrc
		if (GstFlowReturn ret2 = gst_app_src_push_buffer(GST_APP_SRC(*appsrc), buffer); ret2 != GST_FLOW_OK) {
			std::cerr << "Error pushing buffer to appsrc!" << std::endl;
			break;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	// Clean up
	g_object_unref(server);

	return 0;
}
