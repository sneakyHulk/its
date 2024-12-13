#include "StreamingNodeBase.h"

#include <utility>

#include "common_output.h"
#include "gst_rtp_header_extension_timestamp_frame_stream.h"

using namespace std::chrono_literals;

StreamingNodeBase::StreamingNodeBase() {
	if (static bool first = true; std::exchange(first, false)) {
		gst_rtp_header_extension_timestamp_frame_stream_register_static();
	}
}
