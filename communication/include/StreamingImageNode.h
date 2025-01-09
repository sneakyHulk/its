#pragma once

#include <gst/app/gstappsrc.h>
#include <gst/gst.h>
#include <gst/rtp/gstrtphdrext.h>
#include <gst/rtsp-server/rtsp-server.h>

#include <bitset>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <thread>

#include "ImageData.h"
#include "Runner.h"
#include "StreamingNodeBase.h"
#include "common_exception.h"
#include "common_output.h"
#include "timestamp_frame_meta.h"

using namespace std::chrono_literals;

/**
 * @class StreamingImageNode
 * @brief Implements a node that provides an RTSP server for broadcasting images and timestamps.
 *
 * @attention This class have to have a g_main_loop_run or a loop with g_main_context_iteration to be run to function properly. Also, g_main_loop_run or g_main_context_iteration must run in the same thread where this class was created.
 */
class StreamingImageNode : public Runner<ImageData>, StreamingNodeBase {
	inline static GstStaticCaps recording_timestamp_caps = GST_STATIC_CAPS("timestamp-frame/x-stream");
	inline static GstRTSPServer *server = nullptr;
	GstRTSPMediaFactory *factory = nullptr;

	/**
	 * @struct UserData
	 * @brief Holds shared data for the GStreamer pipeline.
	 */
	struct UserData {
		GstElement *source = nullptr;
		GstElement *payloader = nullptr;
		GstElement *header_extension_timestamp_frame = nullptr;
		GstBus *bus = nullptr;
		std::atomic_bool playing = false;
		int width;
		int height;
	} *user_data = new UserData;

   public:
	/**
	 * @brief Constructor that sets up the RTSP server and media factory.
	 */
	StreamingImageNode() {
		// Create an RTSP server only once.
		if (static bool first = true; std::exchange(first, false)) {
			server = gst_rtsp_server_new();
			gst_rtsp_server_set_service(server, "8554");

			// Attach the server to the default main context.
			if (!gst_rtsp_server_attach(server, NULL)) {
				common::println_critical_loc("Failed to attach the RTSP server");
			}
		}

		// Create a factory for the media stream
		factory = gst_rtsp_media_factory_new();

		// The pipeline description using appsrc (rtp payloader has to be named pay0)
		gst_rtsp_media_factory_set_launch(factory, "( appsrc name=bgrsrc ! videoconvert ! video/x-raw,format=I420 ! x264enc speed-preset=ultrafast tune=zerolatency ! rtph264pay name=pay0 pt=96 )");
		gst_rtsp_media_factory_set_shared(factory, true);

		// Attach the factory to the /test endpoint
		GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(server);
		gst_rtsp_mount_points_add_factory(mounts, "/test", factory);
		common::println_loc("RTSP server is running at rtsp://127.0.0.1:8554/test");
		g_object_unref(mounts);
	}
	/**
	 * @brief Destructor that cleans up allocated gstreamer pipeline resources.
	 */
	~StreamingImageNode() {
		if (user_data->payloader) gst_object_unref(user_data->payloader);
		if (user_data->source) gst_object_unref(user_data->source);
		if (user_data->header_extension_timestamp_frame) gst_object_unref(user_data->header_extension_timestamp_frame);
		if (user_data->bus) gst_object_unref(user_data->bus);
		if (factory) gst_object_unref(factory);
		// if (server) gst_object_unref(std::exchange(server, nullptr));

		delete user_data;
	}

   private:
	/**
	 * @brief Initializes and configures the pipeline when a new media/stream is created.
	 * @param factory The media factory.
	 * @param media The RTSP media object.
	 * @param user_data Pointer to user data.
	 */
	static void media_configure([[maybe_unused]] GstRTSPMediaFactory *factory, GstRTSPMedia *media, UserData *user_data) {
		if (user_data->playing.exchange(false)) {
			common::println_warn_loc("Stream should have ended before configuring a new media -> exchanged to end the stream.");  // because factory is shared
		}

		common::println_debug_loc("configure media for streaming!");

		// Retrieve the pipeline and elements
		GstElement *pipeline = gst_rtsp_media_get_element(media);
		GstElement *source = gst_bin_get_by_name_recurse_up(GST_BIN(pipeline), "bgrsrc");
		GstElement *payloader = gst_bin_get_by_name_recurse_up(GST_BIN(pipeline), "pay0");
		GstElement *header_extension_timestamp_frame = gst_element_factory_make("rtp_header_extension_timestamp_frame_stream", "timestamp_frame_stream");
		GstBus *bus = gst_element_get_bus(pipeline);

		// gst_util_set_object_arg(G_OBJECT(source), "format", "time");
		gst_rtp_header_extension_set_id(GST_RTP_HEADER_EXTENSION(header_extension_timestamp_frame), 1);
		g_signal_emit_by_name(payloader, "add-extension", header_extension_timestamp_frame);

		GstCaps *caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "BGR", "width", G_TYPE_INT, user_data->width, "height", G_TYPE_INT, user_data->height, "framerate", GST_TYPE_FRACTION, 30, 1, NULL);
		gst_app_src_set_caps(GST_APP_SRC(source), caps);
		gst_caps_unref(caps);
		// g_object_set(G_OBJECT(source), "format", GST_FORMAT_BUFFERS, "leaky-type", GST_APP_LEAKY_TYPE_DOWNSTREAM, "stream-type", GST_APP_STREAM_TYPE_STREAM, NULL);

		gst_element_set_state(pipeline, GST_STATE_PLAYING);
		gst_object_unref(pipeline);

		if (GstState state; gst_element_get_state(source, &state, nullptr, GST_CLOCK_TIME_NONE) != GST_STATE_CHANGE_SUCCESS || state != GST_STATE_PLAYING) {
			common::println_critical_loc("Pipeline is not in PLAYING state");
		}

		payloader = std::exchange(user_data->payloader, payloader);
		source = std::exchange(user_data->source, source);
		header_extension_timestamp_frame = std::exchange(user_data->header_extension_timestamp_frame, header_extension_timestamp_frame);
		bus = std::exchange(user_data->bus, bus);

		user_data->playing.store(true);

		// Clean up old elements
		if (payloader) gst_object_unref(payloader);
		if (source) gst_object_unref(source);
		if (header_extension_timestamp_frame) gst_object_unref(header_extension_timestamp_frame);
		if (bus) gst_object_unref(bus);
	}

	/**
	 * @brief Processes a single frame of image data to configure the width and height of the stream.
	 * @param data The input image data.
	 */
	void run_once(ImageData const &data) final {
		user_data->width = data.image.cols;
		user_data->height = data.image.rows;

		// Add a callback to set up appsrc
		g_signal_connect(factory, "media-configure", G_CALLBACK(media_configure), user_data);

		run(data);
	}

	/**
	 * @brief Put the provided image data into the stream and look out for error and stream messages.
	 * @param data The input image data.
	 */
	void run(ImageData const &data) final {
		static std::uint64_t framenumber = 0;

		if (!user_data->playing.load()) return;

		// Create a new buffer for each frame (buffer does not need to be called unref on because it gets passed)
		GstBuffer *buffer = gst_buffer_new_allocate(NULL, data.image.total() * data.image.elemSize(), NULL);

		// add timestamp metadata to the stream
		gst_buffer_add_timestamp_frame_meta(buffer, gst_static_caps_get(&recording_timestamp_caps), data.timestamp, framenumber++);

		// Fill the buffer with data
		if (GstMapInfo map; gst_buffer_map(buffer, &map, GST_MAP_WRITE)) {
			std::memcpy(map.data, data.image.data, data.image.total() * data.image.elemSize());

			gst_buffer_unmap(buffer, &map);
		} else {
			common::println_warn_loc("gst_buffer_map failed!");
		}

		// Push buffer to the stream
		GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(user_data->source), buffer);

		// Process messages from the GStreamer
		for (GstMessage *msg = gst_bus_pop(user_data->bus); msg;) {
			switch (GST_MESSAGE_TYPE(msg)) {
				case GST_MESSAGE_UNKNOWN: common::println_loc("GST_MESSAGE_UNKNOWN: An undefined message."); break;
				case GST_MESSAGE_EOS: common::println_loc("GST_MESSAGE_EOS: End-of-stream reached. Applications should stop playback when receiving this in the PLAYING state."); break;
				case GST_MESSAGE_ERROR:
					GError *err;
					gchar *err_info;
					gst_message_parse_error(msg, &err, &err_info);
					common::println_error_loc("GST_MESSAGE_ERROR: Error received from element ", GST_OBJECT_NAME(msg->src), ": ", err->message, "\n Debugging information: ", err_info);
					g_clear_error(&err);
					g_free(err_info);
					break;
				case GST_MESSAGE_WARNING:
					GError *warn;
					gchar *warn_info;
					gst_message_parse_warning(msg, &warn, &warn_info);
					common::println_warn_loc("GST_MESSAGE_WARNING: Warning received from element ", GST_OBJECT_NAME(msg->src), ": ", warn->message, "\n Debugging information: ", warn_info);
					g_clear_error(&err);
					g_free(warn_info);
					break;
				case GST_MESSAGE_INFO: common::println_loc("GST_MESSAGE_INFO: An informational message."); break;
				case GST_MESSAGE_TAG: common::println_loc("GST_MESSAGE_TAG: A metadata tag was found."); break;
				case GST_MESSAGE_BUFFERING: common::println_loc("GST_MESSAGE_BUFFERING: The pipeline is buffering. For non-live pipelines, the application must pause until buffering completes."); break;
				case GST_MESSAGE_STATE_CHANGED:
					GstState old_state, new_state;
					gst_message_parse_state_changed(msg, &old_state, &new_state, NULL);
					common::println_loc("GST_MESSAGE_STATE_CHANGED: Element ", GST_OBJECT_NAME(msg->src), " changed state from ", gst_element_state_get_name(old_state), " to ", gst_element_state_get_name(new_state));
					break;
				case GST_MESSAGE_STATE_DIRTY: common::println_loc("GST_MESSAGE_STATE_DIRTY: An element changed state in a streaming thread (deprecated)."); break;
				case GST_MESSAGE_STEP_DONE: common::println_loc("GST_MESSAGE_STEP_DONE: A stepping operation finished."); break;
				case GST_MESSAGE_CLOCK_PROVIDE: common::println_loc("GST_MESSAGE_CLOCK_PROVIDE: An element provides a clock. This is an internal message."); break;
				case GST_MESSAGE_CLOCK_LOST: common::println_loc("GST_MESSAGE_CLOCK_LOST: The current clock became unusable. Applications should reset the pipeline to PLAYING state after pausing."); break;
				case GST_MESSAGE_NEW_CLOCK: common::println_loc("GST_MESSAGE_NEW_CLOCK: A new clock was selected in the pipeline."); break;
				case GST_MESSAGE_STRUCTURE_CHANGE: common::println_loc("GST_MESSAGE_STRUCTURE_CHANGE: The structure of the pipeline changed (internal)."); break;
				case GST_MESSAGE_STREAM_STATUS:
					common::print_loc("GST_MESSAGE_STREAM_STATUS:");
					GstStreamStatusType stream_status;
					GstElement *owner;
					gst_message_parse_stream_status(msg, &stream_status, &owner);
					switch (stream_status) {
						case GST_STREAM_STATUS_TYPE_CREATE: common::println("Status of a stream of", GST_OBJECT_NAME(msg->src), " is GST_STREAM_STATUS_TYPE_CREATE."); break;
						case GST_STREAM_STATUS_TYPE_ENTER: common::println("Status of a stream of ", GST_OBJECT_NAME(msg->src), " is GST_STREAM_STATUS_TYPE_ENTER."); break;
						case GST_STREAM_STATUS_TYPE_LEAVE: common::println("Status of a stream of ", GST_OBJECT_NAME(msg->src), " is GST_STREAM_STATUS_TYPE_LEAVE."); break;
						case GST_STREAM_STATUS_TYPE_DESTROY: common::println("Status of a stream of ", GST_OBJECT_NAME(msg->src), " is GST_STREAM_STATUS_TYPE_DESTROY."); break;
						case GST_STREAM_STATUS_TYPE_START: common::println("Status of a stream of ", GST_OBJECT_NAME(msg->src), " is GST_STREAM_STATUS_TYPE_START."); break;
						case GST_STREAM_STATUS_TYPE_PAUSE: common::println("Status of a stream of ", GST_OBJECT_NAME(msg->src), " is GST_STREAM_STATUS_TYPE_PAUSE."); break;
						case GST_STREAM_STATUS_TYPE_STOP: common::println("Status of a stream of ", GST_OBJECT_NAME(msg->src), " is GST_STREAM_STATUS_TYPE_STOP."); break;
					}
					break;
				case GST_MESSAGE_APPLICATION: common::println_loc("GST_MESSAGE_APPLICATION: An application-specific message."); break;
				case GST_MESSAGE_ELEMENT: common::println_loc("GST_MESSAGE_ELEMENT: An element-specific message."); break;
				case GST_MESSAGE_SEGMENT_START: common::println_loc("GST_MESSAGE_SEGMENT_START: Playback of a segment started (internal)."); break;
				case GST_MESSAGE_SEGMENT_DONE: common::println_loc("GST_MESSAGE_SEGMENT_DONE: Playback of a segment completed."); break;
				case GST_MESSAGE_DURATION_CHANGED: common::println_loc("GST_MESSAGE_DURATION_CHANGED: The pipeline's duration changed. Applications can query the new duration."); break;
				case GST_MESSAGE_LATENCY: common::println_loc("GST_MESSAGE_LATENCY: The latency of elements changed, requiring recalculation."); break;
				case GST_MESSAGE_ASYNC_START: common::println_loc("GST_MESSAGE_ASYNC_START: An asynchronous state change started (internal)."); break;
				case GST_MESSAGE_ASYNC_DONE: common::println_loc("GST_MESSAGE_ASYNC_DONE: An asynchronous state change completed."); break;
				case GST_MESSAGE_REQUEST_STATE: common::println_loc("GST_MESSAGE_REQUEST_STATE: An element requests a state change in the pipeline."); break;
				case GST_MESSAGE_STEP_START: common::println_loc("GST_MESSAGE_STEP_START: A stepping operation started."); break;
				case GST_MESSAGE_QOS: common::println_loc("GST_MESSAGE_QOS: A buffer was dropped or an element adjusted its processing for Quality of Service reasons."); break;
				case GST_MESSAGE_PROGRESS: common::println_loc("GST_MESSAGE_PROGRESS: A progress-related message."); break;
				case GST_MESSAGE_TOC: common::println_loc("GST_MESSAGE_TOC: A table of contents (TOC) was found or updated."); break;
				case GST_MESSAGE_RESET_TIME: common::println_loc("GST_MESSAGE_RESET_TIME: A request to reset the pipeline's running time (internal)."); break;
				case GST_MESSAGE_STREAM_START: common::println_loc("GST_MESSAGE_STREAM_START: A new stream started. Useful in gapless playback scenarios."); break;
				case GST_MESSAGE_NEED_CONTEXT: common::println_loc("GST_MESSAGE_NEED_CONTEXT: An element requests a specific context."); break;
				case GST_MESSAGE_HAVE_CONTEXT: common::println_loc("GST_MESSAGE_HAVE_CONTEXT: An element created a context."); break;
				case GST_MESSAGE_EXTENDED: common::println_loc("GST_MESSAGE_EXTENDED: Marks an extended message type. Cannot be used directly in mask-based APIs but can be checked specifically."); break;
				case GST_MESSAGE_DEVICE_ADDED: common::println_loc("GST_MESSAGE_DEVICE_ADDED: A GstDevice was added to a GstDeviceProvider."); break;
				case GST_MESSAGE_DEVICE_REMOVED: common::println_loc("GST_MESSAGE_DEVICE_REMOVED: A GstDevice was removed from a GstDeviceProvider."); break;
				case GST_MESSAGE_PROPERTY_NOTIFY: common::println_loc("GST_MESSAGE_PROPERTY_NOTIFY: A GObject property was changed."); break;
				case GST_MESSAGE_STREAM_COLLECTION: common::println_loc("GST_MESSAGE_STREAM_COLLECTION: A new GstStreamCollection is available."); break;
				case GST_MESSAGE_STREAMS_SELECTED: common::println_loc("GST_MESSAGE_STREAMS_SELECTED: The active selection of streams has changed."); break;
				case GST_MESSAGE_REDIRECT: common::println_loc("GST_MESSAGE_REDIRECT: A redirection to another URL was requested."); break;
				case GST_MESSAGE_DEVICE_CHANGED: common::println_loc("GST_MESSAGE_DEVICE_CHANGED: A GstDevice was changed in a GstDeviceProvider."); break;
				case GST_MESSAGE_INSTANT_RATE_REQUEST: common::println_loc("GST_MESSAGE_INSTANT_RATE_REQUEST: Request for an instant rate change, possibly in the past."); break;
				case GST_MESSAGE_ANY: common::println_loc("GST_MESSAGE_ANY: A mask for all messages."); break;
				default: std::unreachable();
			}
			gst_message_unref(msg);

			msg = gst_bus_pop(user_data->bus);
		}

		switch (ret) {
			case GST_FLOW_OK: break;
			case GST_FLOW_FLUSHING:
				common::println_warn_loc("GST_FLOW_FLUSHING: Couldn't push buffer to appsrc. Stream ended?");

				// Stop pushing images to the stream because no one is watching anymore.
				user_data->playing.store(false);
				break;
			case GST_FLOW_CUSTOM_SUCCESS_2:
			case GST_FLOW_CUSTOM_SUCCESS_1:
			case GST_FLOW_CUSTOM_SUCCESS:
			case GST_FLOW_NOT_LINKED:
			case GST_FLOW_EOS:
			case GST_FLOW_NOT_NEGOTIATED:
			case GST_FLOW_ERROR:
			case GST_FLOW_NOT_SUPPORTED:
			case GST_FLOW_CUSTOM_ERROR:
			case GST_FLOW_CUSTOM_ERROR_1:
			case GST_FLOW_CUSTOM_ERROR_2:
			default: common::println_critical_loc("Error pushing buffer to appsrc!, ", ret);
		}
	}
};