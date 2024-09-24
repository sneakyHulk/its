#pragma once

#include "yas.h"

#include <mqtt/client.h>

#include "CompactObject.h"
#include "common_exception.h"
#include "common_output.h"
#include "node.h"

class DataStreamMQTT : public OutputNode<CompactObjects> {
	mqtt::client cli;

   public:
	explicit DataStreamMQTT() : cli(mqtt::client("localhost", +"objects_pub", mqtt::create_options(MQTTVERSION_5))) {
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

	void output_function(CompactObjects const &data) final {
		std::uint64_t now = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now()).time_since_epoch().count();

		yas::mem_ostream os;
		yas::binary_oarchive<yas::mem_ostream> oa(os);
		oa.serialize(data);

		// mqtt::property timestamp(mqtt::property::USER_PROPERTY, "timestamp", std::to_string(now));
		// mqtt::properties proplist;
		// proplist.add(timestamp);
		// cli.publish(mqtt::message("objects", os.get_intrusive_buffer().data, os.get_intrusive_buffer().size, 0, false, proplist));

		cli.publish(mqtt::message("objects", os.get_intrusive_buffer().data, os.get_intrusive_buffer().size, 0, false));
	}
};

class DataInputMQTT : public InputNode<CompactObjects> {
	mqtt::client cli;

   public:
	DataInputMQTT() : cli(mqtt::client("localhost", "objects_sub", mqtt::create_options(MQTTVERSION_5))) {
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
		cli.subscribe("objects", 0, subOpts, props);
	}

	CompactObjects input_function() final {
		auto msg = cli.consume_message();
		auto props = msg->get_properties();

		yas::mem_istream is(msg->get_payload().data(), msg->get_payload().size());
		yas::binary_iarchive<yas::mem_istream> ia(is);

		CompactObjects data;
		ia & data;

		return data;
	}
};