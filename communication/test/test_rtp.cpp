#include <gst/app/gstappsrc.h>
#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>

#include <chrono>
#include <cstdint>
#include <opencv2/opencv.hpp>
#include <thread>
#include <vector>

#include "ImageData.h"
#include "camera_simulator_node.h"
#include "common_exception.h"

using namespace std::chrono_literals;

class RTPTransmitter : public OutputPtrNode<ImageData> {
   public:
	RTPTransmitter() {}

   private:
	void output_function(std::shared_ptr<ImageData const> const &data) final {
		if (!_need_data) std::this_thread::yield();

		int bufferSize = data->image.cols * data->image.rows * 3;
		GstBuffer *buffer = gst_buffer_new_and_alloc(bufferSize);
		GstMapInfo m;
		gst_buffer_map(buffer, &m, GST_MAP_WRITE);
		memcpy(m.data, data->image.data, bufferSize);
		gst_buffer_unmap(buffer, &m);

		GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(appsrc), buffer);
	}
};

std::atomic_bool RTPTransmitter::_need_data{false};
GstElement *RTPTransmitter::appsrc = nullptr;

int main(int argc, char **argv) {
	gst_init(&argc, &argv);

	GMainLoop *loop = g_main_loop_new(NULL, FALSE);

	// ost::RTPSession session(ost::InetHostAddress ("127.0.0.1"), RECEIVER_BASE);
	// common::println("Hello, ", ost::defaultApplication().getSDESItem(ost::SDESItemTypeCNAME), "...");
	// session.setSchedulingTimeout(20000);
	// session.setExpireTimeout(3000000);
	// if (!session.addDestination(ost::InetHostAddress ("127.0.0.1"), TRANSMITTER_BASE)) throw common::Exception("Could not connect to port (", TRANSMITTER_BASE, ").");
	// session.setPayloadFormat(ost::StaticPayloadFormat(ost::sptJPEG));
	// session.startRunning();

	// return 0;
	CameraSimulator cam_n("s110_n_cam_8");
	RTPTransmitter transmitter;

	cam_n += transmitter;

	std::thread cam_n_thread(&CameraSimulator::operator(), &cam_n);
	std::thread transmitter_thread(&RTPTransmitter::operator(), &transmitter);

	g_print("stream ready at rtsp://127.0.0.1:%s/test\n", "8554");
	g_main_loop_run(loop);
}