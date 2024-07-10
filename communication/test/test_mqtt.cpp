#include <mosquitto.h>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <thread>
#include <vector>

#include "ImageData.h"
#include "camera_simulator_node.h"
#include "common_exception.h"

using namespace std::chrono_literals;

class [[maybe_unused]] ImageStreamMQTT : public OutputPtrNode<ImageData> {
	std::shared_ptr<ImageData const> _image_data;
	std::string const cam_name;

   public:
	explicit ImageStreamMQTT(std::string const &cam_name) : cam_name(cam_name) {
		mosquitto_lib_init();

		struct mosquitto *mosq = mosquitto_new("publisher-client", true, NULL);
		if (!mosq) {
			throw common::Exception("Error: Out of memory.");
		}

		mosquitto_connect_callback_set(mosq, ImageStreamMQTT::on_connect);
		mosquitto_publish_callback_set(mosq, ImageStreamMQTT::on_publish);

		int rc = mosquitto_connect(mosq, "localhost", 1883, 60);
		if (rc != MOSQ_ERR_SUCCESS) {
			std::cerr << "Could not connect to Broker with return code " << rc << std::endl;
			throw common::Exception("Could not connect to Broker with return code ", rc, ".");
		}

		mosquitto_loop_start(mosq);
	}

	static void on_connect(struct mosquitto *mosq, void *obj, int rc) {
		if (rc == 0) {
			std::cout << "Connected to broker!" << std::endl;
			const char *message = "Hello, MQTT!";
			mosquitto_publish(mosq, NULL, "test/topic", strlen(message), message, 0, false);
		} else {
			std::cout << "Failed to connect, return code " << rc << std::endl;
		}
	}

	static void on_publish(struct mosquitto *mosq, void *obj, int mid) { std::cout << "Message with mid " << mid << " has been published." << std::endl; }

   private:
	void output_function(std::shared_ptr<ImageData const> const &data) final { std::atomic_store(&_image_data, data); }
};

int main(int argc, char **argv) {
	CameraSimulator cam_n("s110_n_cam_8");
	CameraSimulator cam_o("s110_o_cam_8");
	ImageStreamMQTT transmitter("s110_n_cam_8");
	ImageStreamMQTT transmitter2("s110_o_cam_8");

	cam_n += transmitter;
	cam_o += transmitter2;

	std::thread cam_n_thread(&CameraSimulator::operator(), &cam_n);
	std::thread cam_o_thread(&CameraSimulator::operator(), &cam_o);
	std::thread transmitter_thread(&ImageStreamMQTT::operator(), &transmitter);
	std::thread transmitter2_thread(&ImageStreamMQTT::operator(), &transmitter2);

	for(;;)
		std::this_thread::sleep_for(10s);
}