#include <gst/app/gstappsrc.h>
#include <gst/gst.h>
#include <gst/rtp/gstrtpbuffer.h>
#include <gst/rtp/gstrtphdrext.h>
#include <gst/rtp/rtp.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <gst/video/video.h>

#include <chrono>
#include <opencv2/opencv.hpp>
#include <random>
#include <thread>

#include "gst_rtp_header_extension_random_number.h"
#include "random_number_meta.h"

using namespace std::chrono_literals;

// G_BEGIN_DECLS
// #define GST_TYPE_RTP_HEADER_EXTENSION_RANDOM_NUMBER (gst_rtp_header_extension_random_number_get_type())
// G_DECLARE_FINAL_TYPE(GstRTPHeaderExtensionRandomNumber, gst_rtp_header_extension_random_number, GST, RTP_HEADER_EXTENSION_RANDOM_NUMBER, GstRTPHeaderExtension);
// GST_ELEMENT_REGISTER_DECLARE(rtphdrextrandomnumber);
// G_END_DECLS
//
// #ifndef PACKAGE
// #define PACKAGE "rtphdrextrandomnumber"
// #endif
//
// #ifndef VERSION
// #define VERSION "1.0.0.0"
// #endif
//
// #ifndef GST_PACKAGE_NAME
// #define GST_PACKAGE_NAME "GStreamer"
// #endif
//
// #ifndef GST_PACKAGE_ORIGIN
// #define GST_PACKAGE_ORIGIN "http://somewhere.net/"
// #endif
//
// #define RANDOM_NUMBER_HDR_EXT_URI GST_RTP_HDREXT_BASE "urn:custom:rtp-header-extension:random-number:1.0"
//
// enum {
//	PROP_0,
//	PROP_RANDOM_NUMBER,
// };
//
// struct _GstRTPHeaderExtensionRandomNumber {
//	GstRTPHeaderExtension parent;
//	guint64 random_number;
// };
//
// G_DEFINE_TYPE_WITH_CODE(
//     GstRTPHeaderExtensionRandomNumber, gst_rtp_header_extension_random_number, GST_TYPE_RTP_HEADER_EXTENSION, GST_DEBUG_CATEGORY_INIT(GST_CAT_DEFAULT, "rtphdrextrandomnumber", 0, "RTP Custom Random Number Header Extension"););
// GST_ELEMENT_REGISTER_DEFINE(rtphdrextrandomnumber, "rtphdrextrandomnumber", GST_RANK_MARGINAL, GST_TYPE_RTP_HEADER_EXTENSION_RANDOM_NUMBER);
//
// static GstRTPHeaderExtensionFlags gst_rtp_header_extension_random_number_get_supported_flags(GstRTPHeaderExtension *ext) {
//	return static_cast<GstRTPHeaderExtensionFlags>(GST_RTP_HEADER_EXTENSION_ONE_BYTE | GST_RTP_HEADER_EXTENSION_TWO_BYTE);  // because sizeof(guint64) < 16
// }
// static gsize gst_rtp_header_extension_random_number_get_max_size(GstRTPHeaderExtension *ext, const GstBuffer *buffer) {
//	GstRTPHeaderExtensionRandomNumber *self = GST_RTP_HEADER_EXTENSION_RANDOM_NUMBER(ext);
//	return sizeof(self->random_number);
// }
//
// static gssize gst_rtp_header_extension_random_number_write(GstRTPHeaderExtension *ext, const GstBuffer *input_meta, GstRTPHeaderExtensionFlags write_flags, GstBuffer *output, guint8 *data, gsize size) {
//	GstRTPHeaderExtensionRandomNumber *self = GST_RTP_HEADER_EXTENSION_RANDOM_NUMBER(ext);
//
//	g_return_val_if_fail(size >= gst_rtp_header_extension_random_number_get_max_size(ext, nullptr), -1);
//	g_return_val_if_fail(write_flags & gst_rtp_header_extension_random_number_get_supported_flags(ext), -1);
//
//	GST_OBJECT_LOCK(ext);
//
//	GST_LOG_OBJECT(self, "writing random number \'%lu\'", self->random_number);
//	GST_WRITE_UINT64_BE(data, self->random_number);
//
//	GST_OBJECT_UNLOCK(ext);
//
//	return sizeof(self->random_number);
// }
//
// static gboolean gst_rtp_header_extension_random_number_read(GstRTPHeaderExtension *ext, GstRTPHeaderExtensionFlags read_flags, const guint8 *data, gsize size, GstBuffer *buffer) {
//	GstRTPHeaderExtensionRandomNumber *self = GST_RTP_HEADER_EXTENSION_RANDOM_NUMBER(ext);
//	gboolean notify = FALSE;
//
//	if (!data || size == 0) return TRUE;
//
//	if (read_flags & GST_RTP_HEADER_EXTENSION_ONE_BYTE && (size < 1 || size > 16)) {
//		GST_ERROR_OBJECT(ext, "one-byte header extensions must be between 1 and 16 bytes inculusive");
//		return FALSE;
//	}
//
//	GST_OBJECT_LOCK(self);
//	self->random_number = GST_READ_UINT64_BE(data);
//	GST_LOG_OBJECT(self, "reading random number \'%lu\'", self->random_number);
//	GST_OBJECT_UNLOCK(self);
//
//	g_object_notify((GObject *)self, "random_number");
//
//	return TRUE;
// }
//
// static void gst_rtp_header_extension_random_number_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec) {
//	GstRTPHeaderExtensionRandomNumber *self = GST_RTP_HEADER_EXTENSION_RANDOM_NUMBER(object);
//
//	switch (prop_id) {
//		case PROP_RANDOM_NUMBER:
//			GST_OBJECT_LOCK(self);
//			g_value_set_uint64(value, self->random_number);
//			GST_OBJECT_UNLOCK(self);
//			break;
//		default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec); break;
//	}
// }
//
// static void gst_rtp_header_extension_random_number_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec) {
//	GstRTPHeaderExtensionRandomNumber *self = GST_RTP_HEADER_EXTENSION_RANDOM_NUMBER(object);
//
//	switch (prop_id) {
//		case PROP_RANDOM_NUMBER:
//			GST_OBJECT_LOCK(self);
//			self->random_number = g_value_get_uint64(value);
//			GST_OBJECT_UNLOCK(self);
//			break;
//		default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec); break;
//	}
// }
//
// static void gst_rtp_header_extension_random_number_class_init(GstRTPHeaderExtensionRandomNumberClass *klass) {
//	GstRTPHeaderExtensionClass *rtp_hdr_class;
//	GstElementClass *gstelement_class;
//	GObjectClass *gobject_class;
//
//	rtp_hdr_class = (GstRTPHeaderExtensionClass *)klass;
//	gobject_class = (GObjectClass *)klass;
//	gstelement_class = (GstElementClass *)klass;
//
//	gobject_class->set_property = gst_rtp_header_extension_random_number_set_property;
//	gobject_class->get_property = gst_rtp_header_extension_random_number_get_property;
//
//	g_object_class_install_property(gobject_class, PROP_RANDOM_NUMBER, g_param_spec_uint64("random_number", "Random number", "A random number", 0, G_MAXUINT64, 37ULL, static_cast<GParamFlags>(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
//
//	rtp_hdr_class->get_supported_flags = gst_rtp_header_extension_random_number_get_supported_flags;
//	rtp_hdr_class->get_max_size = gst_rtp_header_extension_random_number_get_max_size;
//	rtp_hdr_class->write = gst_rtp_header_extension_random_number_write;
//	rtp_hdr_class->read = gst_rtp_header_extension_random_number_read;
//
//	gst_element_class_set_static_metadata(gstelement_class, "RTP Custom Header Extension Random Number", GST_RTP_HDREXT_ELEMENT_CLASS,
//	    "Extends RTP packets to add or retrieve a 64-bit random number"
//	    "random number",
//	    "Lukas Heyn <lukas.heyn@gmail.com>");
//	gst_rtp_header_extension_class_set_uri(rtp_hdr_class, GST_RTP_HDREXT_BASE RANDOM_NUMBER_HDR_EXT_URI);
// }
//
// static void gst_rtp_header_extension_random_number_init(GstRTPHeaderExtensionRandomNumber *self) {
//	// self->random_number = 37ULL;
// }
//
// static gboolean plugin_init(GstPlugin *plugin) { return GST_ELEMENT_REGISTER(rtphdrextrandomnumber, plugin); }
//
// GST_PLUGIN_DEFINE(GST_VERSION_MAJOR, GST_VERSION_MINOR, rtphdrextrandomnumber, "Random number RTP Header Extension", plugin_init, VERSION, "LGPL", GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN);

static GstStaticCaps recording_timestamp_caps = GST_STATIC_CAPS("timestamp/x-recording-stream");

constexpr bool get(std::size_t index, std::uint64_t value) { return (value >> index) & 1ULL; }
void add_image_based_timestamp(cv::Mat const &image, std::uint64_t timestamp) {
	auto width = image.cols / 64;
	auto height = std::min(width, image.rows);

	for (auto i = 0; i < 64; ++i) {
		if (get(i, timestamp))
			cv::rectangle(image, cv::Point(i * width, 0), cv::Point(i * width + width, height), cv::Scalar_<int>(255, 255, 255), -1);
		else
			cv::rectangle(image, cv::Point(i * width, 0), cv::Point(i * width + width, height), cv::Scalar_<int>(0, 0, 0), -1);
	}
}

static GstPadProbeReturn extract_metadata_from_frame(GstPad *pad, GstPadProbeInfo *info, gpointer user_data) {
	if (GST_PAD_PROBE_INFO_TYPE(info) & GST_PAD_PROBE_TYPE_BUFFER) {
		GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);
		if (!buffer) {
			return GST_PAD_PROBE_OK;
		}

		GstRandomNumberMeta *meta;
		meta = gst_buffer_get_random_number_meta(buffer, gst_static_caps_get(&recording_timestamp_caps));
		if (meta) {
			std::cout << "RANDOM Number: " << meta->random_number << std::endl;
		}
	}

	return GST_PAD_PROBE_OK;
}

int main(int argc, char *argv[]) {
	gst_init(&argc, &argv);

	gst_rtp_header_extension_random_number_register_static();

	// GST_PLUGIN_STATIC_REGISTER(rtphdrextrandomnumber);
	// gstrtphdrextrandomnumber_register_static();

	cv::VideoCapture images("/home/lukas/Downloads/4865386-uhd_4096_2160_25fps.mp4");

	GstElement *pipeline = gst_pipeline_new("rtp-stream");
	GstElement *source = gst_element_factory_make("appsrc", "source");
	GstElement *videoconvert = gst_element_factory_make("videoconvert", "converter");
	GstElement *filter = gst_element_factory_make("capsfilter", "filter");
	GstElement *encoder = gst_element_factory_make("x264enc", "encoder");
	GstElement *payloader = gst_element_factory_make("rtph264pay", "payloader");

	GstElement *sink = gst_element_factory_make("udpsink", "sink");

	if (!pipeline || !source || !videoconvert || !filter || !encoder || !payloader || !sink) {
		std::cerr << "Failed to create GStreamer elements" << std::endl;
		return -1;
	}

	GstCaps *caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "BGR", "width", G_TYPE_INT, static_cast<int>(images.get(cv::CAP_PROP_FRAME_WIDTH)), "height", G_TYPE_INT,
	    static_cast<int>(images.get(cv::CAP_PROP_FRAME_HEIGHT)), "framerate", GST_TYPE_FRACTION, 30, 1, NULL);
	g_object_set(G_OBJECT(source), "caps", caps, "format", GST_FORMAT_BUFFERS, "leaky-type", GST_APP_LEAKY_TYPE_DOWNSTREAM, "stream-type", GST_APP_STREAM_TYPE_STREAM, NULL);
	gst_caps_unref(caps);

	GstCaps *filter_caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "I420", "width", G_TYPE_INT, static_cast<int>(images.get(cv::CAP_PROP_FRAME_WIDTH)), "height", G_TYPE_INT,
	    static_cast<int>(images.get(cv::CAP_PROP_FRAME_HEIGHT)), "framerate", GST_TYPE_FRACTION, 30, 1, NULL);
	g_object_set(G_OBJECT(filter), "caps", filter_caps, NULL);
	gst_caps_unref(filter_caps);

	gst_util_set_object_arg(G_OBJECT(encoder), "tune", "fastdecode");
	int tune_value;
	g_object_get(G_OBJECT(encoder), "tune", &tune_value, NULL);
	switch (tune_value) {
		case 0x00000001: g_print("Encoder tune value is: Still image\n"); break;
		case 0x00000002: g_print("Encoder tune value is: Fast decode\n"); break;
		case 0x00000004: g_print("Encoder tune value is: Zero latency\n"); break;
		default: std::cerr << "No valid encoder tune value: " << tune_value << std::endl; return -1;
	}

	gst_util_set_object_arg(G_OBJECT(encoder), "psy-tune", "film");
	int psy_tune_value;
	g_object_get(G_OBJECT(encoder), "psy-tune", &psy_tune_value, NULL);
	switch (psy_tune_value) {
		case 0: g_print("Encoder psy tune value is: No tuning\n"); break;
		case 1: g_print("Encoder psy tune value is: Film\n"); break;
		case 2: g_print("Encoder psy tune value is: Animation\n"); break;
		case 3: g_print("Encoder psy tune value is: Grain\n"); break;
		case 4: g_print("Encoder psy tune value is: PSNR\n"); break;
		case 5: g_print("Encoder psy tune value is: SSIM\n"); break;
		default: std::cerr << "No valid encoder psy tune value: " << psy_tune_value << std::endl; return -1;
	}

	gst_util_set_object_arg(G_OBJECT(encoder), "pass", "qual");

	gst_util_set_object_arg(G_OBJECT(encoder), "speed-preset", "medium");
	int preset_value;
	g_object_get(G_OBJECT(encoder), "speed-preset", &preset_value, NULL);
	switch (preset_value) {
		case 0: g_print("Encoder speed-preset value is: None"); break;
		case 1: g_print("Encoder speed-preset value is: ultrafast"); break;
		case 2: g_print("Encoder speed-preset value is: superfast\n"); break;
		case 3: g_print("Encoder speed-preset value is: veryfast\n"); break;
		case 4: g_print("Encoder speed-preset value is: faster\n"); break;
		case 5: g_print("Encoder speed-preset value is: fast\n"); break;
		case 6: g_print("Encoder speed-preset value is: medium\n"); break;
		case 7: g_print("Encoder speed-preset value is: slow\n"); break;
		case 8: g_print("Encoder speed-preset value is: slower\n"); break;
		case 9: g_print("Encoder speed-preset value is: veryslow\n"); break;
		case 10: g_print("Encoder speed-preset value is:placebo\n"); break;
		default: std::cerr << "No valid encoder speed-preset value: " << preset_value << std::endl; return -1;
	}

	g_object_set(G_OBJECT(payloader), "pt", 96, NULL);

	GstRTPHeaderExtension *random_number = reinterpret_cast<GstRTPHeaderExtension *>(gst_element_factory_make("rtp_header_extension_random_number", "random_number"));

	// auto ext = gst_rtp_header_extension_create_from_uri(GST_RTP_HDREXT_BASE RANDOM_NUMBER_HDR_EXT_URI);
	gst_rtp_header_extension_set_id(random_number, 1);
	g_signal_emit_by_name(payloader, "add-extension", random_number);

	g_object_set(G_OBJECT(sink), "host", "127.0.0.1", "port", 5005, "sync", FALSE, NULL);

	GstPad *sink_pad = gst_element_get_static_pad(payloader, "src");
	gst_pad_add_probe(sink_pad, GST_PAD_PROBE_TYPE_BUFFER, extract_metadata_from_frame, NULL, NULL);
	gst_object_unref(sink_pad);

	gst_bin_add_many(GST_BIN(pipeline), source, videoconvert, filter, encoder, payloader, sink, NULL);

	if (!gst_element_link_many(source, videoconvert, filter, encoder, payloader, sink, NULL)) {
		std::cerr << "Failed to link elements" << std::endl;
		gst_object_unref(pipeline);
		return -1;
	}

	GstBus *bus = gst_element_get_bus(pipeline);

	// Start the pipeline
	if (GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING); ret == GST_STATE_CHANGE_FAILURE) {
		std::cerr << "Failed to start the pipeline" << std::endl;
		gst_object_unref(pipeline);
		return -1;
	}

	guint64 timestamp = 0;
	for (guint64 frame_count = 0, timestamp = 0;;) {
		images = cv::VideoCapture("/home/lukas/Downloads/4865386-uhd_4096_2160_25fps.mp4");
		for (int i = 0; i < static_cast<int>(images.get(cv::CAP_PROP_FRAME_COUNT)); std::cout << "Frame: " << ++i << std::endl) {
			cv::Mat frame;
			images >> frame;

			add_image_based_timestamp(frame, std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count());

			cv::putText(frame, std::to_string(i), cv::Point2d(500, 500), cv::FONT_HERSHEY_DUPLEX, 5.0, cv::Scalar_<int>(0, 0, 0), 5);

			std::cout << " read" << std::flush;

			// std::vector<uint8_t> sei;
			//// NAL unit start code and type for SEI
			// sei.push_back(0x00);
			// sei.push_back(0x00);
			// sei.push_back(0x00);
			// sei.push_back(0x01);  // NAL unit start code
			// sei.push_back(0x06);  // NAL unit type for SEI
			//
			//// Payload type (user-defined) and size
			// sei.push_back(5);  // Payload type (example: 5 for user data unregistered)
			// sei.push_back(8);  // Payload size (frame count and timestamp, 8 bytes total)
			//
			//// Add frame count (4 bytes)
			// sei.push_back((frame_count >> 24) & 0xFF);
			// sei.push_back((frame_count >> 16) & 0xFF);
			// sei.push_back((frame_count >> 8) & 0xFF);
			// sei.push_back(frame_count & 0xFF);
			//
			//// Add timestamp (4 bytes, assuming lower 32 bits for simplicity)
			// sei.push_back((timestamp >> 24) & 0xFF);
			// sei.push_back((timestamp >> 16) & 0xFF);
			// sei.push_back((timestamp >> 8) & 0xFF);
			// sei.push_back(timestamp & 0xFF);
			//
			//++timestamp;
			//++frame_count;
			//
			// RBSP (trailing bits) and padding
			// sei.push_back(0x80);  // RBSP stop bit

			GstBuffer *buffer = gst_buffer_new_allocate(NULL, frame.total() * frame.elemSize(), NULL);
			// GstBuffer *buffer = gst_buffer_new_allocate(NULL, frame.total() * frame.elemSize() + sei.size(), NULL);

			// buffer->pts = 1000;

			gst_buffer_add_random_number_meta(buffer, gst_static_caps_get(&recording_timestamp_caps), timestamp++);

			//  GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, 10);

			std::cout << " pts" << std::flush;

			GstMapInfo map;
			if (gst_buffer_map(buffer, &map, GST_MAP_WRITE)) {
				std::memcpy(map.data, frame.data, frame.total() * frame.elemSize());
				// std::memcpy(map.data + frame.total() * frame.elemSize(), sei.data(), sei.size());

				gst_buffer_unmap(buffer, &map);
			}

			std::cout << " map" << std::flush;

			// Push buffer to appsrc
			if (GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(source), buffer); ret != GST_FLOW_OK) {
				std::cerr << "Error pushing buffer to appsrc!" << std::endl;
				break;
			}

			std::cout << " push " << std::flush;

			// Check bus for messages
			for (GstMessage *msg = gst_bus_pop(bus); msg;) {
				switch (GST_MESSAGE_TYPE(msg)) {
					case GST_MESSAGE_UNKNOWN:
						std::cout << "GST_MESSAGE_UNKNOWN:" << std::flush;
						g_print("An undefined message.\n");
						break;
					case GST_MESSAGE_EOS:
						std::cout << "GST_MESSAGE_EOS:" << std::flush;
						g_print("End-of-stream reached. Applications should stop playback when receiving this in the PLAYING state.\n");
						break;
					case GST_MESSAGE_ERROR:
						std::cout << "GST_MESSAGE_ERROR:" << std::flush;
						GError *err;
						gchar *err_info;
						gst_message_parse_error(msg, &err, &err_info);
						g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
						g_printerr("Debugging information: %s\n", err_info);
						g_clear_error(&err);
						g_free(err_info);
						break;
					case GST_MESSAGE_WARNING:
						std::cout << "GST_MESSAGE_WARNING:" << std::flush;
						GError *warn;
						gchar *warn_info;
						gst_message_parse_warning(msg, &warn, &warn_info);
						g_printerr("Warning received from element %s: %s\n", GST_OBJECT_NAME(msg->src), warn->message);
						g_printerr("Debugging information: %s\n", warn_info);
						g_clear_error(&err);
						g_free(warn_info);
						break;
					case GST_MESSAGE_INFO:
						std::cout << "GST_MESSAGE_INFO:" << std::flush;
						g_print("An informational message.\n");
						break;
					case GST_MESSAGE_TAG:
						std::cout << "GST_MESSAGE_TAG:" << std::flush;
						g_print("A metadata tag was found.\n");
						break;
					case GST_MESSAGE_BUFFERING:
						std::cout << "GST_MESSAGE_BUFFERING:" << std::flush;
						g_print("The pipeline is buffering. For non-live pipelines, the application must pause until buffering completes.\n");
						break;
					case GST_MESSAGE_STATE_CHANGED:
						std::cout << "GST_MESSAGE_STATE_CHANGED:" << std::flush;
						GstState old_state, new_state;
						gst_message_parse_state_changed(msg, &old_state, &new_state, NULL);
						g_print("Element %s changed state from %s to %s.\n", GST_OBJECT_NAME(msg->src), gst_element_state_get_name(old_state), gst_element_state_get_name(new_state));
						break;
					case GST_MESSAGE_STATE_DIRTY:
						std::cout << "GST_MESSAGE_STATE_DIRTY:" << std::flush;
						g_print("An element changed state in a streaming thread (deprecated).\n");
						break;
					case GST_MESSAGE_STEP_DONE:
						std::cout << "GST_MESSAGE_STEP_DONE:" << std::flush;
						g_print("A stepping operation finished.\n");
						break;
					case GST_MESSAGE_CLOCK_PROVIDE:
						std::cout << "GST_MESSAGE_CLOCK_PROVIDE:" << std::flush;
						g_print("An element provides a clock. This is an internal message.\n");
						break;
					case GST_MESSAGE_CLOCK_LOST:
						std::cout << "GST_MESSAGE_CLOCK_LOST:" << std::flush;
						g_print("The current clock became unusable. Applications should reset the pipeline to PLAYING state after pausing.\n");
						break;
					case GST_MESSAGE_NEW_CLOCK:
						std::cout << "GST_MESSAGE_NEW_CLOCK:" << std::flush;
						g_print("A new clock was selected in the pipeline.\n");
						break;
					case GST_MESSAGE_STRUCTURE_CHANGE:
						std::cout << "GST_MESSAGE_STRUCTURE_CHANGE:" << std::flush;
						g_print("The structure of the pipeline changed (internal).\n");
						break;
					case GST_MESSAGE_STREAM_STATUS:
						std::cout << "GST_MESSAGE_STREAM_STATUS:" << std::flush;
						GstStreamStatusType stream_status;
						GstElement *owner;
						gst_message_parse_stream_status(msg, &stream_status, &owner);
						switch (stream_status) {
							case GST_STREAM_STATUS_TYPE_CREATE:
								std::cout << "GST_STREAM_STATUS_TYPE_CREATE:" << std::flush;
								g_print("Status of a stream of %s is GST_STREAM_STATUS_TYPE_CREATE.\n", GST_OBJECT_NAME(msg->src));
								break;
							case GST_STREAM_STATUS_TYPE_ENTER:
								std::cout << "GST_STREAM_STATUS_TYPE_ENTER:" << std::flush;
								g_print("Status of a stream of %s is GST_STREAM_STATUS_TYPE_ENTER.\n", GST_OBJECT_NAME(msg->src));
								break;
							case GST_STREAM_STATUS_TYPE_LEAVE:
								std::cout << "GST_STREAM_STATUS_TYPE_LEAVE:" << std::flush;
								g_print("Status of a stream of %s is GST_STREAM_STATUS_TYPE_LEAVE.\n", GST_OBJECT_NAME(msg->src));
								break;
							case GST_STREAM_STATUS_TYPE_DESTROY:
								std::cout << "GST_STREAM_STATUS_TYPE_DESTROY:" << std::flush;
								g_print("Status of a stream of %s is GST_STREAM_STATUS_TYPE_DESTROY.\n", GST_OBJECT_NAME(msg->src));
								break;
							case GST_STREAM_STATUS_TYPE_START:
								std::cout << "GST_STREAM_STATUS_TYPE_START:" << std::flush;
								g_print("Status of a stream of %s is GST_STREAM_STATUS_TYPE_START.\n", GST_OBJECT_NAME(msg->src));
								break;
							case GST_STREAM_STATUS_TYPE_PAUSE:
								std::cout << "GST_STREAM_STATUS_TYPE_PAUSE:" << std::flush;
								g_print("Status of a stream of %s is GST_STREAM_STATUS_TYPE_PAUSE.\n", GST_OBJECT_NAME(msg->src));
								break;
							case GST_STREAM_STATUS_TYPE_STOP:
								std::cout << "GST_STREAM_STATUS_TYPE_STOP:" << std::flush;
								g_print("Status of a stream of %s is GST_STREAM_STATUS_TYPE_STOP.\n", GST_OBJECT_NAME(msg->src));
								break;
						}
						break;
					case GST_MESSAGE_APPLICATION:
						std::cout << "GST_MESSAGE_APPLICATION:" << std::flush;
						g_print("An application-specific message.\n");
						break;
					case GST_MESSAGE_ELEMENT:
						std::cout << "GST_MESSAGE_ELEMENT:" << std::flush;
						g_print("An element-specific message.\n");
						break;
					case GST_MESSAGE_SEGMENT_START:
						std::cout << "GST_MESSAGE_SEGMENT_START:" << std::flush;
						g_print("Playback of a segment started (internal).\n");
						break;
					case GST_MESSAGE_SEGMENT_DONE:
						std::cout << "GST_MESSAGE_SEGMENT_DONE:" << std::flush;
						g_print("Playback of a segment completed.\n");
						break;
					case GST_MESSAGE_DURATION_CHANGED:
						std::cout << "GST_MESSAGE_DURATION_CHANGED:" << std::flush;
						g_print("The pipeline's duration changed. Applications can query the new duration.\n");
						break;
					case GST_MESSAGE_LATENCY:
						std::cout << "GST_MESSAGE_LATENCY:" << std::flush;
						g_print("The latency of elements changed, requiring recalculation.\n");
						break;
					case GST_MESSAGE_ASYNC_START:
						std::cout << "GST_MESSAGE_ASYNC_START:" << std::flush;
						g_print("An asynchronous state change started (internal).\n");
						break;
					case GST_MESSAGE_ASYNC_DONE:
						std::cout << "GST_MESSAGE_ASYNC_DONE:" << std::flush;
						g_print("An asynchronous state change completed.\n");
						break;
					case GST_MESSAGE_REQUEST_STATE:
						std::cout << "GST_MESSAGE_REQUEST_STATE:" << std::flush;
						g_print("An element requests a state change in the pipeline.\n");
						break;
					case GST_MESSAGE_STEP_START:
						std::cout << "GST_MESSAGE_STEP_START:" << std::flush;
						g_print("A stepping operation started.\n");
						break;
					case GST_MESSAGE_QOS:
						std::cout << "GST_MESSAGE_QOS:" << std::flush;
						g_print("A buffer was dropped or an element adjusted its processing for Quality of Service reasons.\n");
						break;
					case GST_MESSAGE_PROGRESS:
						std::cout << "GST_MESSAGE_PROGRESS:" << std::flush;
						g_print("A progress-related message.\n");
						break;
					case GST_MESSAGE_TOC:
						std::cout << "GST_MESSAGE_TOC:" << std::flush;
						g_print("A table of contents (TOC) was found or updated.\n");
						break;
					case GST_MESSAGE_RESET_TIME:
						std::cout << "GST_MESSAGE_RESET_TIME:" << std::flush;
						g_print("A request to reset the pipeline's running time (internal).\n");
						break;
					case GST_MESSAGE_STREAM_START:
						std::cout << "GST_MESSAGE_STREAM_START:" << std::flush;
						g_print("A new stream started. Useful in gapless playback scenarios.\n");
						break;
					case GST_MESSAGE_NEED_CONTEXT:
						std::cout << "GST_MESSAGE_NEED_CONTEXT:" << std::flush;
						g_print("An element requests a specific context.\n");
						break;
					case GST_MESSAGE_HAVE_CONTEXT:
						std::cout << "GST_MESSAGE_HAVE_CONTEXT:" << std::flush;
						g_print("An element created a context.\n");
						break;
					case GST_MESSAGE_EXTENDED:
						std::cout << "GST_MESSAGE_EXTENDED:" << std::flush;
						g_print("Marks an extended message type. Cannot be used directly in mask-based APIs but can be checked specifically.\n");
						break;
					case GST_MESSAGE_DEVICE_ADDED:
						std::cout << "GST_MESSAGE_DEVICE_ADDED:" << std::flush;
						g_print("A GstDevice was added to a GstDeviceProvider.\n");
						break;
					case GST_MESSAGE_DEVICE_REMOVED:
						std::cout << "GST_MESSAGE_DEVICE_REMOVED:" << std::flush;
						g_print("A GstDevice was removed from a GstDeviceProvider.\n");
						break;
					case GST_MESSAGE_PROPERTY_NOTIFY:
						std::cout << "GST_MESSAGE_PROPERTY_NOTIFY:" << std::flush;
						g_print("A GObject property was changed.\n");
						break;
					case GST_MESSAGE_STREAM_COLLECTION:
						std::cout << "GST_MESSAGE_STREAM_COLLECTION:" << std::flush;
						g_print("A new GstStreamCollection is available.\n");
						break;
					case GST_MESSAGE_STREAMS_SELECTED:
						std::cout << "GST_MESSAGE_STREAMS_SELECTED:" << std::flush;
						g_print("The active selection of streams has changed.\n");
						break;
					case GST_MESSAGE_REDIRECT:
						std::cout << "GST_MESSAGE_REDIRECT:" << std::flush;
						g_print("A redirection to another URL was requested.\n");
						break;
					case GST_MESSAGE_DEVICE_CHANGED:
						std::cout << "GST_MESSAGE_DEVICE_CHANGED:" << std::flush;
						g_print("A GstDevice was changed in a GstDeviceProvider.\n");
						break;
					case GST_MESSAGE_INSTANT_RATE_REQUEST:
						std::cout << "GST_MESSAGE_INSTANT_RATE_REQUEST:" << std::flush;
						g_print("Request for an instant rate change, possibly in the past.\n");
						break;
					case GST_MESSAGE_ANY:
						std::cout << "GST_MESSAGE_ANY:" << std::flush;
						g_print("A mask for all messages.\n");
						break;
					default:
						std::cout << "default:" << std::flush;
						g_print("lol.\n");
						break;
				}
				gst_message_unref(msg);
				std::cout << " unref" << std::flush;

				msg = gst_bus_pop(bus);
				std::cout << " pop " << std::flush;
			}

			std::cout << "end" << std::endl;

			g_main_context_iteration(NULL, false);

			g_usleep(33333);
		}
	}

	gst_object_unref(bus);
	gst_element_set_state(pipeline, GST_STATE_NULL);
	gst_object_unref(pipeline);

	return 0;
}