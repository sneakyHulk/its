#include <ccrtp/rtp.h>

#include <chrono>
#include <cstdint>
#include <opencv2/opencv.hpp>
#include <thread>
#include <vector>

#include "ImageData.h"
#include "camera_simulator_node.h"
#include "common_exception.h"

using namespace std::chrono_literals;

class [[maybe_unused]] ImageStreamUDP : public OutputPtrNode<ImageData> {
	std::shared_ptr<ImageData const> _image_data;
	std::string const cam_name;
	ost::RTPSession *socket;
	ost::InetHostAddress local_ip = "127.0.0.1";

   public:
	ImageStreamUDP(std::string const &cam_name) : cam_name(cam_name) {}

   private:
	void run(std::shared_ptr<ImageData const> const &data) final { std::atomic_store(&_image_data, data); }
};

int main(int argc, char **argv) {
	CameraSimulator cam_n("s110_n_cam_8");
	CameraSimulator cam_o("s110_o_cam_8");
	ImageStreamUDP transmitter("s110_n_cam_8");
	ImageStreamUDP transmitter2("s110_o_cam_8");

	cam_n += transmitter;
	cam_o += transmitter2;

	std::thread cam_n_thread(&CameraSimulator::operator(), &cam_n);
	std::thread cam_o_thread(&CameraSimulator::operator(), &cam_o);
	std::thread transmitter_thread(&ImageStreamUDP::operator(), &transmitter);
	std::thread transmitter2_thread(&ImageStreamUDP::operator(), &transmitter2);
}