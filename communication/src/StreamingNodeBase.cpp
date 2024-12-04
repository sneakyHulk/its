#include "StreamingNodeBase.h"

#include <gst/gst.h>

#include <thread>
#include <utility>

#include "common_output.h"
#include "gst_rtp_header_extension_timestamp_frame_stream.h"

StreamingNodeBase::StreamingNodeBase() {
	if (static bool first = true; std::exchange(first, false)) {
		gst_rtp_header_extension_timestamp_frame_stream_register_static();
		// Start the server
		loop_thread = std::thread([]() {
			GMainLoop *loop = g_main_loop_new(NULL, FALSE);
			common::println_loc("RTSP server is running at rtsp://127.0.0.1:8554/test");
			g_main_loop_run(loop);

			g_main_loop_unref(loop);
		});
	}
}
