#include "ReceivingDataNode2.h"

#include "common_output.h"
#include "yas.h"

/**
 * @brief Configure MQTT connection and connect to the broker and subscribe to the topic.
 */
ReceivingDataNode2::ReceivingDataNode2() : cli(mqtt::client("localhost", "objects_sub", mqtt::create_options(MQTTVERSION_5))) {
	auto connOpts = mqtt::connect_options_builder()
	                    .mqtt_version(MQTTVERSION_5)
	                    .automatic_reconnect(std::chrono::seconds(2), std::chrono::seconds(30))
	                    .clean_start(false)
	                    .properties({{mqtt::property::SESSION_EXPIRY_INTERVAL, std::numeric_limits<uint32_t>::max()}})
	                    .finalize();

	common::print_loc("Connecting to the MQTT server...");
	mqtt::connect_response rsp = cli.connect(connOpts);
	common::println("OK");

	mqtt::subscribe_options subOpts;
	mqtt::properties props;
	cli.subscribe("objects", 0, subOpts, props);
}

/**
 * @brief Consumes and deserializes the next message from the MQTT topic "objects".
 *
 * @return A CompactObjects instance reconstructed from the received payload.
 */
CompactObjects ReceivingDataNode2::push() {
	auto msg = cli.consume_message();
	auto props = msg->get_properties();

	yas::mem_istream is(msg->get_payload().data(), msg->get_payload().size());
	yas::binary_iarchive<yas::mem_istream> ia(is);

	CompactObjects data;
	ia & data;

	return data;
}