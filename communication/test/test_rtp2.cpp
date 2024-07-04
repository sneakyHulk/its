#include <ccrtp/rtp.h>

#include <chrono>
#include <cstdint>
#include <opencv2/opencv.hpp>
#include <vector>

#include "ImageData.h"
#include "camera_simulator_node.h"
#include "common_exception.h"

using namespace std::chrono_literals;

const int RECEIVER_BASE = 33634;
const int TRANSMITTER_BASE = 32522;

class RTPTransmitter : public OutputPtrNode<ImageData> {
	ost::RTPSession session;
	std::vector<std::uint8_t> buf;

   public:
	RTPTransmitter() : session(ost::InetHostAddress("127.0.0.1")) {
		common::println("Hello, ", ost::defaultApplication().getSDESItem(ost::SDESItemTypeCNAME), "...");
		session.setSchedulingTimeout(20000);
		session.setExpireTimeout(3000000);
		if (!session.addDestination(ost::InetHostAddress("127.0.0.1"), RECEIVER_BASE)) throw common::Exception("Could not connect to port (", ").");

		session.setPayloadFormat(ost::StaticPayloadFormat(ost::sptJPEG));
		session.startRunning();
	}

   private:
	void output_function(std::shared_ptr<ImageData const> const& data) final {
		// static thread_local std::uint32_t timestamp = session.getCurrentTimestamp();

		cv::imencode(".jpg", data->image, buf, {cv::IMWRITE_JPEG_QUALITY, 90});

		session.putData(session.getCurrentTimestamp(), buf.data(), buf.size());
	}
};

int main() {
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

	ost::RTPSession session(ost::InetHostAddress("127.0.0.1"));
	session.setSchedulingTimeout(20000);
	session.setExpireTimeout(3000000);
	if (!session.addDestination(ost::InetHostAddress("127.0.0.1"), TRANSMITTER_BASE)) throw common::Exception("Could not connect to port (", ").");

	session.setPayloadFormat(ost::StaticPayloadFormat(ost::sptJPEG));
	session.startRunning();

	while (true) {
		ost::AppDataUnit const* data_unit = NULL;
		while (data_unit == NULL) {
			std::this_thread::sleep_for(10ms);
			data_unit = session.getData(session.getCurrentTimestamp());
		}

		auto data = data_unit->getData();
		for (auto i = 0; i < data_unit->getSize(); ++i) {
			common::print(data[i]);
		}
		common::println();
	}
}