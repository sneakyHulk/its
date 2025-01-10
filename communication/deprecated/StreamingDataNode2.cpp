#include "StreamingDataNode.h"

#include "common_output.h"
#include "yas.h"

/**
 * @brief Configure MQTT connection and connect to the broker
 */
StreamingDataNode::StreamingDataNode() : cli(mqtt::client("localhost", +"objects_pub", mqtt::create_options(MQTTVERSION_5))) {
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

/**
 * @brief Publishes serialized compact object data to the MQTT topic "objects".
 *
 * @param data The CompactObjects instance to serialize and publish.
 */
void StreamingDataNode::run(CompactObjects const& data) {
	yas::mem_ostream os;
	yas::binary_oarchive<yas::mem_ostream> oa(os);
	oa.serialize(data);

	// std::uint64_t now = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now()).time_since_epoch().count();
	// mqtt::property timestamp(mqtt::property::USER_PROPERTY, "timestamp", std::to_string(now));
	// mqtt::properties proplist;
	// proplist.add(timestamp);
	// cli.publish(mqtt::message("objects", os.get_intrusive_buffer().data, os.get_intrusive_buffer().size, 0, false, proplist));

	cli.publish(mqtt::message("objects", os.get_intrusive_buffer().data, os.get_intrusive_buffer().size, 0, false));
}