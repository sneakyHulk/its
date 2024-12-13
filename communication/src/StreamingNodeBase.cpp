#include "StreamingNodeBase.h"

#include <utility>

#include "gst_rtp_header_extension_timestamp_frame_stream.h"

StreamingNodeBase::StreamingNodeBase() {
	if (static bool first = true; std::exchange(first, false)) {
		gst_rtp_header_extension_timestamp_frame_stream_register_static();
	}
}
