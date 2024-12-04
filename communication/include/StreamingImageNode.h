#pragma once

#include <gst/app/gstappsrc.h>
#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>

#include <bitset>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <string>
#include <thread>

#include "ImageData.h"
#include "common_exception.h"
#include "common_output.h"
#include "node.h"

class StreamingImageNode : public Runner<ImageData> {
	inline static GstRTSPServer *server = nullptr;
	inline static bool first = true;
	inline static std::thread loop_thread;

	GstRTSPMediaFactory *factory;

	struct UserData {
		GstElement *source = nullptr;
		GstElement *payloader = nullptr;
		GstBus *bus = nullptr;
		std::atomic_flag playing = ATOMIC_FLAG_INIT;
		int width = 320;
		int height = 240;
	} *user_data = new UserData;

	int width = 320;
	int height = 240;
	int size = width * height * 3;  // RGB
	guint8 *datamem = new guint8[size];

   public:
	StreamingImageNode() {
		common::println("\033[4;31mbold red text\033[0m\n", "test", common::LOGLEVEL::ERROR);

		// Create an RTSP server only once
		if (!server) {
			server = gst_rtsp_server_new();
			gst_rtsp_server_set_service(server, "8554");
		}

		// Create a factory for the media stream
		factory = gst_rtsp_media_factory_new();
		// The pipeline description using appsrc (rtp payloader has to be named pay0)
		gst_rtsp_media_factory_set_launch(
		    factory, "( appsrc name=bgrsrc is-live=true format=time emit-signals=false ! videoconvert ! video/x-raw,format=I420 ! x264enc speed-preset=ultrafast tune=zerolatency ! rtph264pay name=pay0 pt=96 )");
		gst_rtsp_media_factory_set_shared(factory, true);

		// Attach the factory to the /test endpoint
		GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(server);
		gst_rtsp_mount_points_add_factory(mounts, "/test", factory);
		g_object_unref(mounts);

		// Start the server
		if (std::exchange(first, false)) {
			loop_thread = std::thread([]() {
				/* attach the server to the default maincontext */
				if (!gst_rtsp_server_attach(server, NULL)) {
					throw common::Exception("Failed to attach the RTSP server");
				}

				GMainLoop *loop = g_main_loop_new(NULL, FALSE);
				common::println("RTSP server is running at rtsp://127.0.0.1:8554/test");
				g_main_loop_run(loop);

				g_main_loop_unref(loop);
				g_object_unref(server);
			});
		}
	}
	~StreamingImageNode() {
		if (user_data->payloader) gst_object_unref(user_data->payloader);
		if (user_data->source) gst_object_unref(user_data->source);
		if (user_data->bus) gst_object_unref(user_data->bus);

		delete user_data;
	}

   private:
	static void media_configure(GstRTSPMediaFactory *, GstRTSPMedia *media, UserData *user_data) {
		GstElement *pipeline = gst_rtsp_media_get_element(media);
		GstElement *source = gst_bin_get_by_name_recurse_up(GST_BIN(pipeline), "bgrsrc");
		GstElement *payloader = gst_bin_get_by_name_recurse_up(GST_BIN(pipeline), "pay0");
		GstBus *bus = gst_element_get_bus(pipeline);

		GstCaps *caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "BGR", "width", G_TYPE_INT, 320, "height", G_TYPE_INT, 240, "framerate", GST_TYPE_FRACTION, 30, 1, NULL);
		gst_app_src_set_caps(GST_APP_SRC(source), caps);
		gst_caps_unref(caps);

		gst_element_set_state(pipeline, GST_STATE_PLAYING);
		gst_object_unref(pipeline);

		if (GstState state; gst_element_get_state(source, &state, nullptr, GST_CLOCK_TIME_NONE) != GST_STATE_CHANGE_SUCCESS || state != GST_STATE_PLAYING) {
			throw common::Exception("Pipeline is not in PLAYING state");
		}

		payloader = std::exchange(user_data->payloader, payloader);
		source = std::exchange(user_data->source, source);
		bus = std::exchange(user_data->bus, bus);

		user_data->playing.test_and_set();

		if (payloader) gst_object_unref(payloader);
		if (source) gst_object_unref(source);
		if (bus) gst_object_unref(bus);
	}

	void run_once(ImageData const &data) final {
		user_data->width = data.image.cols;
		user_data->height = data.image.rows;

		// Add a callback to set up appsrc
		g_signal_connect(factory, "media-configure", G_CALLBACK(media_configure), user_data);
	}

	guint64 timestamp = 0;

	void run(ImageData const &data) final {
		if (!user_data->playing.test_and_set()) {
			user_data->playing.clear();
			return;
		}

		std::memset(datamem, 0, size);  // Fill with black

		// Create a new buffer for each frame
		GstBuffer *buffer = gst_buffer_new_allocate(NULL, size, NULL);

		// Fill the buffer with data
		GstMapInfo map;
		gst_buffer_map(buffer, &map, GST_MAP_WRITE);
		std::memcpy(map.data, datamem, size);
		gst_buffer_unmap(buffer, &map);

		// Push buffer to appsrc
		switch (GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(user_data->source), buffer); ret) {
			case GST_FLOW_OK:
				for (GstMessage *msg = gst_bus_pop(user_data->bus); msg;) {
					switch (GST_MESSAGE_TYPE(msg)) {
						case GST_MESSAGE_UNKNOWN: common::println("GST_MESSAGE_UNKNOWN: An undefined message.\n"); break;
						case GST_MESSAGE_EOS: common::println("GST_MESSAGE_EOS: End-of-stream reached. Applications should stop playback when receiving this in the PLAYING state.\n"); break;
						case GST_MESSAGE_ERROR:
							GError *err;
							gchar *err_info;
							gst_message_parse_error(msg, &err, &err_info);
							common::println("GST_MESSAGE_ERROR: Error received from element ", GST_OBJECT_NAME(msg->src), ": ", err->message, "\n Debugging information: ", err_info);
							g_clear_error(&err);
							g_free(err_info);
							break;
						case GST_MESSAGE_WARNING:
							GError *warn;
							gchar *warn_info;
							gst_message_parse_warning(msg, &warn, &warn_info);
							common::println("GST_MESSAGE_WARNING: Warning received from element ", GST_OBJECT_NAME(msg->src), ": ", warn->message, "\n Debugging information: ", warn_info);
							g_clear_error(&err);
							g_free(warn_info);
							break;
						case GST_MESSAGE_INFO: common::println("GST_MESSAGE_INFO: An informational message.\n"); break;
						case GST_MESSAGE_TAG: common::println("GST_MESSAGE_TAG: A metadata tag was found.\n"); break;
						case GST_MESSAGE_BUFFERING: common::println("GST_MESSAGE_BUFFERING: The pipeline is buffering. For non-live pipelines, the application must pause until buffering completes.\n"); break;
						case GST_MESSAGE_STATE_CHANGED:
							common::println("GST_MESSAGE_STATE_CHANGED:");
							GstState old_state, new_state;
							gst_message_parse_state_changed(msg, &old_state, &new_state, NULL);
							common::println("GST_MESSAGE_STATE_CHANGED: Element ", GST_OBJECT_NAME(msg->src), " changed state from ", gst_element_state_get_name(old_state), " to ", gst_element_state_get_name(new_state));
							break;
						case GST_MESSAGE_STATE_DIRTY: common::println("GST_MESSAGE_STATE_DIRTY: An element changed state in a streaming thread (deprecated).\n"); break;
						case GST_MESSAGE_STEP_DONE: common::println("GST_MESSAGE_STEP_DONE: A stepping operation finished.\n"); break;
						case GST_MESSAGE_CLOCK_PROVIDE: common::println("GST_MESSAGE_CLOCK_PROVIDE: An element provides a clock. This is an internal message.\n"); break;
						case GST_MESSAGE_CLOCK_LOST: common::println("GST_MESSAGE_CLOCK_LOST: The current clock became unusable. Applications should reset the pipeline to PLAYING state after pausing.\n"); break;
						case GST_MESSAGE_NEW_CLOCK: common::println("GST_MESSAGE_NEW_CLOCK: A new clock was selected in the pipeline.\n"); break;
						case GST_MESSAGE_STRUCTURE_CHANGE: common::println("GST_MESSAGE_STRUCTURE_CHANGE: The structure of the pipeline changed (internal).\n"); break;
						case GST_MESSAGE_STREAM_STATUS:
							common::println("GST_MESSAGE_STREAM_STATUS:");
							GstStreamStatusType stream_status;
							GstElement *owner;
							gst_message_parse_stream_status(msg, &stream_status, &owner);
							switch (stream_status) {
								case GST_STREAM_STATUS_TYPE_CREATE:
									common::println("GST_STREAM_STATUS_TYPE_CREATE:");
									g_print("Status of a stream of %s is GST_STREAM_STATUS_TYPE_CREATE.\n", GST_OBJECT_NAME(msg->src));
									break;
								case GST_STREAM_STATUS_TYPE_ENTER:
									common::println("GST_STREAM_STATUS_TYPE_ENTER:");
									g_print("Status of a stream of %s is GST_STREAM_STATUS_TYPE_ENTER.\n", GST_OBJECT_NAME(msg->src));
									break;
								case GST_STREAM_STATUS_TYPE_LEAVE:
									common::println("GST_STREAM_STATUS_TYPE_LEAVE:");
									g_print("Status of a stream of %s is GST_STREAM_STATUS_TYPE_LEAVE.\n", GST_OBJECT_NAME(msg->src));
									break;
								case GST_STREAM_STATUS_TYPE_DESTROY:
									common::println("GST_STREAM_STATUS_TYPE_DESTROY:");
									g_print("Status of a stream of %s is GST_STREAM_STATUS_TYPE_DESTROY.\n", GST_OBJECT_NAME(msg->src));
									break;
								case GST_STREAM_STATUS_TYPE_START:
									common::println("GST_STREAM_STATUS_TYPE_START:");
									g_print("Status of a stream of %s is GST_STREAM_STATUS_TYPE_START.\n", GST_OBJECT_NAME(msg->src));
									break;
								case GST_STREAM_STATUS_TYPE_PAUSE:
									common::println("GST_STREAM_STATUS_TYPE_PAUSE:");
									g_print("Status of a stream of %s is GST_STREAM_STATUS_TYPE_PAUSE.\n", GST_OBJECT_NAME(msg->src));
									break;
								case GST_STREAM_STATUS_TYPE_STOP:
									common::println("GST_STREAM_STATUS_TYPE_STOP:");
									g_print("Status of a stream of %s is GST_STREAM_STATUS_TYPE_STOP.\n", GST_OBJECT_NAME(msg->src));
									break;
							}
							break;
						case GST_MESSAGE_APPLICATION: common::println("GST_MESSAGE_APPLICATION: An application-specific message.\n"); break;
						case GST_MESSAGE_ELEMENT: common::println("GST_MESSAGE_ELEMENT: An element-specific message.\n"); break;
						case GST_MESSAGE_SEGMENT_START: common::println("GST_MESSAGE_SEGMENT_START: Playback of a segment started (internal).\n"); break;
						case GST_MESSAGE_SEGMENT_DONE: common::println("GST_MESSAGE_SEGMENT_DONE: Playback of a segment completed.\n"); break;
						case GST_MESSAGE_DURATION_CHANGED: common::println("GST_MESSAGE_DURATION_CHANGED: The pipeline's duration changed. Applications can query the new duration.\n"); break;
						case GST_MESSAGE_LATENCY: common::println("GST_MESSAGE_LATENCY: The latency of elements changed, requiring recalculation.\n"); break;
						case GST_MESSAGE_ASYNC_START: common::println("GST_MESSAGE_ASYNC_START: An asynchronous state change started (internal).\n"); break;
						case GST_MESSAGE_ASYNC_DONE: common::println("GST_MESSAGE_ASYNC_DONE: An asynchronous state change completed.\n"); break;
						case GST_MESSAGE_REQUEST_STATE: common::println("GST_MESSAGE_REQUEST_STATE: An element requests a state change in the pipeline.\n"); break;
						case GST_MESSAGE_STEP_START: common::println("GST_MESSAGE_STEP_START: A stepping operation started.\n"); break;
						case GST_MESSAGE_QOS: common::println("GST_MESSAGE_QOS: A buffer was dropped or an element adjusted its processing for Quality of Service reasons.\n"); break;
						case GST_MESSAGE_PROGRESS: common::println("GST_MESSAGE_PROGRESS: A progress-related message.\n"); break;
						case GST_MESSAGE_TOC: common::println("GST_MESSAGE_TOC: A table of contents (TOC) was found or updated.\n"); break;
						case GST_MESSAGE_RESET_TIME: common::println("GST_MESSAGE_RESET_TIME: A request to reset the pipeline's running time (internal).\n"); break;
						case GST_MESSAGE_STREAM_START: common::println("GST_MESSAGE_STREAM_START: A new stream started. Useful in gapless playback scenarios.\n"); break;
						case GST_MESSAGE_NEED_CONTEXT: common::println("GST_MESSAGE_NEED_CONTEXT: An element requests a specific context.\n"); break;
						case GST_MESSAGE_HAVE_CONTEXT: common::println("GST_MESSAGE_HAVE_CONTEXT: An element created a context.\n"); break;
						case GST_MESSAGE_EXTENDED: common::println("GST_MESSAGE_EXTENDED: Marks an extended message type. Cannot be used directly in mask-based APIs but can be checked specifically.\n"); break;
						case GST_MESSAGE_DEVICE_ADDED: common::println("GST_MESSAGE_DEVICE_ADDED: A GstDevice was added to a GstDeviceProvider.\n"); break;
						case GST_MESSAGE_DEVICE_REMOVED: common::println("GST_MESSAGE_DEVICE_REMOVED: A GstDevice was removed from a GstDeviceProvider.\n"); break;
						case GST_MESSAGE_PROPERTY_NOTIFY: common::println("GST_MESSAGE_PROPERTY_NOTIFY: A GObject property was changed.\n"); break;
						case GST_MESSAGE_STREAM_COLLECTION: common::println("GST_MESSAGE_STREAM_COLLECTION: A new GstStreamCollection is available.\n"); break;
						case GST_MESSAGE_STREAMS_SELECTED: common::println("GST_MESSAGE_STREAMS_SELECTED: The active selection of streams has changed.\n"); break;
						case GST_MESSAGE_REDIRECT: common::println("GST_MESSAGE_REDIRECT: A redirection to another URL was requested.\n"); break;
						case GST_MESSAGE_DEVICE_CHANGED: common::println("GST_MESSAGE_DEVICE_CHANGED: A GstDevice was changed in a GstDeviceProvider.\n"); break;
						case GST_MESSAGE_INSTANT_RATE_REQUEST: common::println("GST_MESSAGE_INSTANT_RATE_REQUEST: Request for an instant rate change, possibly in the past.\n"); break;
						case GST_MESSAGE_ANY:
							common::println("GST_MESSAGE_ANY:");
							g_print("A mask for all messages.\n");
							break;
						default: std::unreachable();
					}
					gst_message_unref(msg);

					msg = gst_bus_pop(user_data->bus);
				}
				break;
			case GST_FLOW_FLUSHING:
			case GST_FLOW_CUSTOM_SUCCESS_2: break;
			case GST_FLOW_CUSTOM_SUCCESS_1: break;
			case GST_FLOW_CUSTOM_SUCCESS: break;
			case GST_FLOW_NOT_LINKED: throw common::Exception("Error pushing buffer to appsrc GST_FLOW_NOT_LINKED!");
			case GST_FLOW_EOS: throw common::Exception("Error pushing buffer to appsrc GST_FLOW_EOS!");
			case GST_FLOW_NOT_NEGOTIATED: throw common::Exception("Error pushing buffer to appsrc GST_FLOW_NOT_NEGOTIATED!");
			case GST_FLOW_ERROR: throw common::Exception("Error pushing buffer to appsrc GST_FLOW_ERROR!");
			case GST_FLOW_NOT_SUPPORTED: break;
			case GST_FLOW_CUSTOM_ERROR: break;
			case GST_FLOW_CUSTOM_ERROR_1: break;
			case GST_FLOW_CUSTOM_ERROR_2: break;
			default: throw common::Exception("Error pushing buffer to appsrc!");
		}
	}
};