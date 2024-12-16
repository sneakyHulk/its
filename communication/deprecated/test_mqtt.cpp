#include <mosquitto.h>
#include <mqtt_protocol.h>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <thread>
#include <vector>

#include "BirdEyeVisualizationNode.h"
#include "ImageData.h"
#include "camera_simulator_node.h"
#include "common_exception.h"
#include "mqtt/client.h"

using namespace std::chrono_literals;

class [[maybe_unused]] ImageStreamMQTT : public Runner<ImageData> {
	std::string const cam_name;
	mqtt::client cli;

   public:
	explicit ImageStreamMQTT(std::string const &cam_name) : cam_name(cam_name), cli(mqtt::client("localhost", cam_name + "_pub", mqtt::create_options(MQTTVERSION_5))) {
		auto connOpts = mqtt::connect_options_builder()
		                    .mqtt_version(MQTTVERSION_5)
		                    .automatic_reconnect(std::chrono::seconds(2), std::chrono::seconds(30))
		                    .clean_start(false)
		                    .properties({{mqtt::property::SESSION_EXPIRY_INTERVAL, std::numeric_limits<uint32_t>::max()}})
		                    .finalize();

		common::print("Connecting to the MQTT server...");
		mqtt::connect_response rsp = cli.connect(connOpts);
		common::println("OK");
	}

	void run(ImageData const &data) final {
		std::uint64_t now = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now()).time_since_epoch().count();
		std::vector<std::uint8_t> jpeg_data;
		cv::imencode(".jpeg", data.image, jpeg_data);

		mqtt::property mime(mqtt::property::CONTENT_TYPE, "image/jpeg");
		mqtt::property timestamp(mqtt::property::USER_PROPERTY, "timestamp", std::to_string(now));
		mqtt::properties proplist;
		proplist.add(mime);
		proplist.add(timestamp);

		cli.publish(mqtt::message(data.source + "/image", jpeg_data.data(), jpeg_data.size(), 0, false, proplist));
	}
};

class ImageInputMQTT : public InputNode<ImageData> {
	std::string const cam_name;
	mqtt::client cli;

   public:
	ImageInputMQTT(std::string const &cam_name) : cam_name(cam_name), cli(mqtt::client("localhost", cam_name + "_sub", mqtt::create_options(MQTTVERSION_5))) {
		auto connOpts = mqtt::connect_options_builder()
		                    .mqtt_version(MQTTVERSION_5)
		                    .automatic_reconnect(std::chrono::seconds(2), std::chrono::seconds(30))
		                    .clean_start(false)
		                    .properties({{mqtt::property::SESSION_EXPIRY_INTERVAL, std::numeric_limits<uint32_t>::max()}})
		                    .finalize();

		common::print("Connecting to the MQTT server...");
		mqtt::connect_response rsp = cli.connect(connOpts);
		common::println("OK");

		mqtt::subscribe_options subOpts;
		mqtt::properties props;
		cli.subscribe(cam_name + "/image", 0, subOpts, props);
	}

	ImageData input_function() final {
		auto msg = cli.consume_message();
		auto props = msg->get_properties();

		auto img = cv::imdecode({msg->get_payload().data(), static_cast<int>(msg->get_payload().size())}, cv::IMREAD_COLOR);

		ImageData data(img, std::stoll(props.get(mqtt::property::USER_PROPERTY).c_struct().value.value.data), 1920, 1200, cam_name);

		return data;
	}
};

int main(int argc, char **argv) {
	CameraSimulator cam_n("s110_n_cam_8");
	CameraSimulator cam_o("s110_o_cam_8");
	ImageStreamMQTT transmitter("s110_n_cam_8");
	ImageStreamMQTT transmitter2("s110_o_cam_8");
	ImageInputMQTT receiver("s110_n_cam_8");
	ImageVisualization displayer;

	cam_n += transmitter;
	cam_o += transmitter2;

	receiver += displayer;

	std::thread cam_n_thread(&CameraSimulator::operator(), &cam_n);
	std::thread cam_o_thread(&CameraSimulator::operator(), &cam_o);
	std::thread transmitter_thread(&ImageStreamMQTT::operator(), &transmitter);
	std::thread transmitter2_thread(&ImageStreamMQTT::operator(), &transmitter2);
	std::thread receiver_thread(&ImageInputMQTT::operator(), &receiver);
	std::thread displayer_thread(&ImageVisualization::operator(), &displayer);

	for (;;) std::this_thread::sleep_for(10s);
}