#pragma once

#include <mosquitto.h>

#include <chrono>

#include "CompactObject.h"
#include "Runner.h"
#include "common_output.h"
#include "yas.h"

using namespace std::chrono_literals;

/**
 * @class StreamingDataNode
 * @brief A class responsible for handling streaming data and publishing it to an MQTT broker.
 */
class StreamingDataNode : public Runner<CompactObjects> {
	struct mosquitto *mosq;

   public:
	explicit StreamingDataNode() {
		if (mosq = mosquitto_new("objects_pub", true, this); !mosq) {
			common::println_critical_loc("Failed to create mosquitto instance!");
		}

		common::print_loc("Connecting to the MQTT server...");

		if (mosquitto_connect(mosq, "localhost", 1883, 20) != MOSQ_ERR_SUCCESS) {
			common::println_critical_loc("Failed to connect to the MQTT server!");
		}

		common::println("OK");
	}

	~StreamingDataNode() {
		mosquitto_disconnect(mosq);
		mosquitto_destroy(mosq);
	}

	void run(CompactObjects const &data) final {
		yas::mem_ostream os;
		yas::binary_oarchive<yas::mem_ostream> oa(os);
		oa.serialize(data);

		if (int ret = mosquitto_publish(mosq, nullptr, "objects", os.get_intrusive_buffer().size, os.get_intrusive_buffer().data, 0, false); ret != MOSQ_ERR_SUCCESS) {
			common::println_warn_loc("Failed to publish message: ", mosquitto_strerror(ret), '!');
		}

		for (;;) {
			if (int ret2 = mosquitto_loop(mosq, 1000, 1); ret2 != MOSQ_ERR_SUCCESS) {
				common::println_warn_loc("Failed to handle the network loop: ", mosquitto_strerror(ret2), '!');

				common::print_loc("Reconnecting to the MQTT server...");
				std::this_thread::sleep_for(1s);

				if (int ret = mosquitto_reconnect(mosq); ret != MOSQ_ERR_SUCCESS) {
					common::println_warn("Failed to reconnect to the server: ", mosquitto_strerror(ret), '!');
					continue;
				}

				common::println("OK");
			}

			return;
		}
	}
};