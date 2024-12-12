#pragma once

#include <gst/app/gstappsink.h>
#include <gst/gst.h>
#include <gst/rtp/gstrtphdrext.h>
#include <gst/sdp/gstsdpmessage.h>

#include <chrono>

#include "ImageData.h"
#include "Pusher.h"
#include "StreamingNodeBase.h"
#include "gst_rtp_header_extension_timestamp_frame_stream.h"
#include "timestamp_frame_meta.h"

using namespace std::chrono_literals;

// gst-launch-1.0 --gst-debug=2 rtspsrc location=rtsp://127.0.0.1:8554/test latency=20 udp-reconnect=true ! rtph264depay ! h264parse ! avdec_h264 ! video/x-raw,format=I420 ! videoconvert ! autovideosink sync=false
class ReceivingImageNode : public Pusher<ImageData>, StreamingNodeBase {
	inline static GstStaticCaps timestamp_frame_stream_caps = GST_STATIC_CAPS("timestamp-frame/x-stream");

	GstElement *pipeline;
	GstElement *source;
	GstElement *depayloader;
	GstElement *header_extension_timestamp_frame;
	GstElement *parser;
	GstElement *decoder;
	GstElement *videoconvert;
	GstElement *filter1;
	GstElement *filter2;
	GstElement *sink;
	GstBus *bus;

   public:
	ReceivingImageNode() {
		// Create pipeline and elements
		pipeline = gst_pipeline_new("receiver-pipeline");

		source = gst_element_factory_make("rtspsrc", "source");
		depayloader = gst_element_factory_make("rtph264depay", "depayloader");
		header_extension_timestamp_frame = gst_element_factory_make("rtp_header_extension_timestamp_frame_stream", "timestamp_frame_stream");
		parser = gst_element_factory_make("h264parse", "parser");
		decoder = gst_element_factory_make("avdec_h264", "decoder");
		filter1 = gst_element_factory_make("capsfilter", "filter1");
		videoconvert = gst_element_factory_make("videoconvert", "converter");
		filter2 = gst_element_factory_make("capsfilter", "filter2");
		sink = gst_element_factory_make("appsink", "sink");
		bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));

		if (!pipeline || !source || !depayloader || !header_extension_timestamp_frame || !parser || !decoder || !filter1 || !videoconvert || !filter2 || !sink) {
			common::println_critical_loc("Failed to create elements.");
		}

		g_object_set(source, "location", "rtsp://127.0.0.1:8554/test", "latency", 20, "buffer-mode", 0, NULL);

		gst_rtp_header_extension_set_id(GST_RTP_HEADER_EXTENSION(header_extension_timestamp_frame), 1);
		g_signal_emit_by_name(depayloader, "add-extension", header_extension_timestamp_frame);

		GstCaps *filter1_caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "I420", NULL);
		g_object_set(G_OBJECT(filter1), "caps", filter1_caps, NULL);
		gst_caps_unref(filter1_caps);

		GstCaps *filter2_caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "BGR", NULL);
		g_object_set(G_OBJECT(filter2), "caps", filter2_caps, NULL);
		gst_caps_unref(filter2_caps);

		g_object_set(G_OBJECT(sink), "sync", FALSE, "max-buffers", 1, "drop", TRUE, NULL);

		gst_bin_add_many(GST_BIN(pipeline), source, depayloader, parser, decoder, filter1, videoconvert, filter2, sink, NULL);
		if (!gst_element_link_many(depayloader, parser, decoder, filter1, videoconvert, filter2, sink, NULL)) {
			common::println_critical_loc("Failed to link elements.");
		}

		// Link the dynamic pad for rtspsrc
		g_signal_connect(source, "pad-added", G_CALLBACK(+[](GstElement *source, GstPad *src_pad, GstElement *depayloader) {
			GstPad *sink_pad = gst_element_get_static_pad(depayloader, "sink");
			if (gst_pad_is_linked(sink_pad)) {
				gst_object_unref(sink_pad);
				return;
			}

			if (gst_pad_link(src_pad, sink_pad) != GST_PAD_LINK_OK) {
				gst_object_unref(sink_pad);

				common::println_critical_loc("Failed to link rtspsrc src pad to depayloader sink pad");
			}
			gst_object_unref(sink_pad);
		}),
		    depayloader);

		g_signal_connect(source, "on-sdp", G_CALLBACK(+[](GstElement *, GstSDPMessage *message, gpointer) { common::println_loc(gst_sdp_message_as_text(message)); }), nullptr);

		// Start the pipeline
		if (GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING); ret == GST_STATE_CHANGE_FAILURE) {
			common::println_critical_loc("Failed to set pipeline to PLAYING state.\n");
		}

		if (GstState state; gst_element_get_state(source, &state, nullptr, GST_CLOCK_TIME_NONE) != GST_STATE_CHANGE_SUCCESS || state != GST_STATE_PLAYING) {
			common::println_critical_loc("Pipeline is not in PLAYING state");
		}
	}

	~ReceivingImageNode() { gst_object_unref(pipeline); }

   private:
	ImageData push_once() final { return push(); }

	void reconnect() {
		// Start the pipeline
		if (GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_NULL); ret == GST_STATE_CHANGE_FAILURE) {
			common::println_critical_loc("Failed to set pipeline to PAUSED state.\n");
		}

		{
			GstState state;
			int ret = gst_element_get_state(source, &state, nullptr, GST_CLOCK_TIME_NONE);

			if (ret != GST_STATE_CHANGE_SUCCESS && ret != GST_STATE_CHANGE_NO_PREROLL) {
				common::println_critical_loc("Pipeline is not in PAUSED state");
			}

			if (state != GST_STATE_NULL) {
				common::println_critical_loc("Pipeline is not in PAUSED state");
			}
		}

		if (GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING); ret == GST_STATE_CHANGE_FAILURE) {
			common::println_critical_loc("Failed to set pipeline to PLAYING state.\n");
		}

		if (GstState state; gst_element_get_state(source, &state, nullptr, GST_CLOCK_TIME_NONE) != GST_STATE_CHANGE_SUCCESS || state != GST_STATE_PLAYING) {
			common::println_critical_loc("Pipeline is not in PLAYING state");
		}
	}

	ImageData push() final {
		for (;;) {
			// Exit on EOS
			if (gst_app_sink_is_eos(GST_APP_SINK(sink))) common::println_critical_loc("Stream ended! (EOS)");

			for (GstMessage *msg = gst_bus_pop(bus); msg;) {
				switch (GST_MESSAGE_TYPE(msg)) {
					case GST_MESSAGE_UNKNOWN: common::println_loc("GST_MESSAGE_UNKNOWN: An undefined message."); break;
					case GST_MESSAGE_EOS: common::println_loc("GST_MESSAGE_EOS: End-of-stream reached. Applications should stop playback when receiving this in the PLAYING state."); break;
					case GST_MESSAGE_ERROR:
						GError *err;
						gchar *err_info;
						gst_message_parse_error(msg, &err, &err_info);

						if (err->domain == GST_CORE_ERROR) {
							switch (err->code) {
								case GST_CORE_ERROR_FAILED:
								case GST_CORE_ERROR_TOO_LAZY:
								case GST_CORE_ERROR_NOT_IMPLEMENTED:
								case GST_CORE_ERROR_STATE_CHANGE:
								case GST_CORE_ERROR_PAD:
								case GST_CORE_ERROR_THREAD:
								case GST_CORE_ERROR_NEGOTIATION:
								case GST_CORE_ERROR_EVENT:
								case GST_CORE_ERROR_SEEK:
								case GST_CORE_ERROR_CAPS:
								case GST_CORE_ERROR_TAG:
								case GST_CORE_ERROR_MISSING_PLUGIN:
								case GST_CORE_ERROR_CLOCK:
								case GST_CORE_ERROR_DISABLED:
								case GST_CORE_ERROR_NUM_ERRORS:
								default: common::println_critical_loc("GST_MESSAGE_ERROR: Error received from element ", GST_OBJECT_NAME(msg->src), ": ", err->message, "\n Debugging information: ", err_info);
							}
						} else if (err->domain == GST_LIBRARY_ERROR) {
							switch (err->code) {
								case GST_LIBRARY_ERROR_FAILED:
								case GST_LIBRARY_ERROR_TOO_LAZY:
								case GST_LIBRARY_ERROR_INIT:
								case GST_LIBRARY_ERROR_SHUTDOWN:
								case GST_LIBRARY_ERROR_SETTINGS:
								case GST_LIBRARY_ERROR_ENCODE:
								case GST_LIBRARY_ERROR_NUM_ERRORS:
								default: common::println_critical_loc("GST_MESSAGE_ERROR: Error received from element ", GST_OBJECT_NAME(msg->src), ": ", err->message, "\n Debugging information: ", err_info);
							}
						} else if (err->domain == GST_RESOURCE_ERROR) {
							switch (err->code) {
								case GST_RESOURCE_ERROR_READ:
								case GST_RESOURCE_ERROR_WRITE:
								case GST_RESOURCE_ERROR_OPEN_READ:
								case GST_RESOURCE_ERROR_OPEN_WRITE:
								case GST_RESOURCE_ERROR_OPEN_READ_WRITE: reconnect(); break;
								case GST_RESOURCE_ERROR_FAILED:
								case GST_RESOURCE_ERROR_TOO_LAZY:
								case GST_RESOURCE_ERROR_NOT_FOUND:
								case GST_RESOURCE_ERROR_BUSY:
								case GST_RESOURCE_ERROR_CLOSE:
								case GST_RESOURCE_ERROR_SEEK:
								case GST_RESOURCE_ERROR_SYNC:
								case GST_RESOURCE_ERROR_SETTINGS:
								case GST_RESOURCE_ERROR_NO_SPACE_LEFT:
								case GST_RESOURCE_ERROR_NOT_AUTHORIZED:
								case GST_RESOURCE_ERROR_NUM_ERRORS:
								default: common::println_critical_loc("GST_MESSAGE_ERROR: Error received from element ", GST_OBJECT_NAME(msg->src), ": ", err->message, "\n Debugging information: ", err_info);
							}
						} else if (err->domain == GST_STREAM_ERROR) {
							switch (err->code) {
								case GST_STREAM_ERROR_FAILED:
								case GST_STREAM_ERROR_TOO_LAZY:
								case GST_STREAM_ERROR_NOT_IMPLEMENTED:
								case GST_STREAM_ERROR_TYPE_NOT_FOUND:
								case GST_STREAM_ERROR_WRONG_TYPE:
								case GST_STREAM_ERROR_CODEC_NOT_FOUND:
								case GST_STREAM_ERROR_DECODE:
								case GST_STREAM_ERROR_ENCODE:
								case GST_STREAM_ERROR_DEMUX:
								case GST_STREAM_ERROR_MUX:
								case GST_STREAM_ERROR_FORMAT:
								case GST_STREAM_ERROR_DECRYPT:
								case GST_STREAM_ERROR_DECRYPT_NOKEY:
								case GST_STREAM_ERROR_NUM_ERRORS:
								default: common::println_critical_loc("GST_MESSAGE_ERROR: Error received from element ", GST_OBJECT_NAME(msg->src), ": ", err->message, "\n Debugging information: ", err_info);
							}
						} else {
							common::println_critical_loc("GST_MESSAGE_ERROR: Error received from element ", GST_OBJECT_NAME(msg->src), ": ", err->message, "\n Debugging information: ", err_info);
						}

						g_clear_error(&err);
						g_free(err_info);
						break;
					case GST_MESSAGE_WARNING:
						GError *warn;
						gchar *warn_info;
						gst_message_parse_warning(msg, &warn, &warn_info);
						common::println_warn_loc("GST_MESSAGE_WARNING: Warning received from element ", GST_OBJECT_NAME(msg->src), ": ", warn->message, "\n Debugging information: ", warn_info);
						g_clear_error(&warn);
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

				msg = gst_bus_pop(bus);
			}

			GstSample *sample = gst_app_sink_try_pull_sample(GST_APP_SINK(sink), 10000);
			if (!sample) continue;

			GstCaps *caps = gst_sample_get_caps(sample);
			auto structure = gst_caps_get_structure(GST_CAPS(caps), 0);
			int width, height;
			gst_structure_get_int(structure, "width", &width);
			gst_structure_get_int(structure, "height", &height);
			auto format = gst_structure_get_string(structure, "format");
			common::println_loc("Sample: Width = ", width, ", Height = ", height, ", Format = ", format);

			GstBuffer *buffer = gst_sample_get_buffer(sample);
			GstMapInfo map;
			if (!gst_buffer_map(buffer, &map, GST_MAP_READ)) {
				common::println_error_loc("Error with mapping the gstreamer buffer!");
				continue;
			}

			common::println_loc("Received frame of size: ", map.size, " bytes");
			// if (map.size != (height + height / 2) * width) {
			if (map.size != height * width * 3) {
				common::println_error_loc("Not enough data received! Buffer size '", map.size, "' != '", (height + height / 2) * width, "' the expected size.");
				continue;
			}

			ImageData ret{cv::Mat(height, width, CV_8UC3), 0, "test"};
			std::memcpy(ret.image.data, map.data, map.size);

			// cv::Mat yuv_img(height + height / 2, width, CV_8UC1, map.data);
			// cv::cvtColor(yuv_img, ret.image, cv::COLOR_YUV2BGR_I420);

			GstTimestampFrameMeta *meta;
			meta = gst_buffer_get_timestamp_frame_meta(buffer, gst_static_caps_get(&timestamp_frame_stream_caps));
			if (!meta) {
				common::println_warn_loc("Metadata not found!");
			} else {
				ret.timestamp = meta->timestamp;
			}
			std::chrono::nanoseconds duration = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()) - std::chrono::nanoseconds(ret.timestamp);
			common::println_loc(duration.count());

			gst_buffer_unmap(buffer, &map);
			gst_sample_unref(sample);

			return ret;
		}

		// Pull the sample (synchronous, wait)
		// GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));
		// if (!sample) common::println_critical_loc("Stream has no sample!");

		// Get width and height from sample caps (NOT element caps)

		// static GstBuffer *buffer = nullptr;
		// buffer = gst_sample_get_buffer(sample);

		// GstMapInfo map;
		// if (!gst_buffer_map(buffer, &map, GST_MAP_READ)) common::println_critical_loc("Error with mapping the gstreamer buffer!");
		//
		// common::println_loc("Received frame of size: ", map.size, " bytes");
		//
		// if (map.size != (height + height / 2) * width)
		//	;
		// cv::Mat const yuv_img(height + height / 2, width, CV_8UC1, map.data);
		// ret.image = cv::Mat(height, width, CV_8UC3);
		// cv::cvtColor(yuv_img, ret.image, cv::COLOR_YUV2BGR_I420);
	}
};