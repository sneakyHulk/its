#include "StreamingNodeBase.h"

#include <gst/gst.h>

#include <thread>
#include <utility>

#include "common_output.h"
#include "gst_rtp_header_extension_timestamp_frame_stream.h"
using namespace std::chrono_literals;

StreamingNodeBase::StreamingNodeBase() {
	if (static bool first = true; std::exchange(first, false)) {
		gst_rtp_header_extension_timestamp_frame_stream_register_static();

		// Start the server
		loop_thread = std::thread([]() {
			for (;; std::this_thread::yield()) g_main_context_iteration(NULL, true);

			// GMainLoop *loop = g_main_loop_new(NULL, FALSE);
			// g_main_loop_run(loop);
			//
			// g_main_loop_unref(loop);
		});
	}
}
