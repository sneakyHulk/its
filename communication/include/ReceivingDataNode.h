#pragma once

#include <mosquitto.h>

#include <chrono>

#include "CompactObject.h"
#include "Pusher.h"
#include "common_output.h"
#include "yas.h"

using namespace std::chrono_literals;

/**
 * @class ReceivingDataNode
 * @brief A class responsible for subscribing to an MQTT topic and deserializing received data.
 */
class ReceivingDataNode : public Pusher<CompactObjects> {
	struct mosquitto *mosq;

	CompactObjects data;

   public:
	/**
	 * @brief Configure MQTT connection and connect to the broker and subscribe to the topic.
	 */
	ReceivingDataNode() {
		if (mosq = mosquitto_new("objects_sub", true, this); !mosq) {
			common::println_critical_loc("Failed to create mosquitto instance!");
		}

		mosquitto_message_callback_set(mosq, onMessage);

		common::print_loc("Connecting to the MQTT server...");

		if (mosquitto_connect(mosq, "localhost", 1883, 20) != MOSQ_ERR_SUCCESS) {
			common::println_critical_loc("Failed to connect to the MQTT server!");
		}

		// Subscribe to the topic
		if (mosquitto_subscribe(mosq, nullptr, "objects", 0) != MOSQ_ERR_SUCCESS) {
			common::println_critical_loc("Failed to subscribe to the topic!");
		}

		common::println("OK");
	}

	~ReceivingDataNode() {
		mosquitto_disconnect(mosq);
		mosquitto_destroy(mosq);
	}

	static void onMessage(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg) {
		if (!msg) {
			common::println_warn_loc("No message!");

			return;
		}

		if (!msg->payload) {
			common::println_warn_loc("No payload!");

			return;
		}

		auto *node = static_cast<ReceivingDataNode *>(obj);

		yas::mem_istream is(msg->payload, msg->payloadlen);
		yas::binary_iarchive<yas::mem_istream> ia(is);

		ia & node->data;
	}

	/**
	 * @brief Consumes and deserializes the next message from the MQTT topic "objects".
	 *
	 * @return A CompactObjects instance reconstructed from the received payload.
	 */
	CompactObjects push() final {
		for (;;) {
			if (int ret2 = mosquitto_loop(mosq, 1000, 1); ret2 != MOSQ_ERR_SUCCESS) {
				common::println_warn_loc("Failed to handle the network loop: ", mosquitto_strerror(ret2), '!');

				common::print_loc("Reconnecting to the MQTT server...");
				std::this_thread::sleep_for(1s);

				if (int ret = mosquitto_reconnect(mosq); ret != MOSQ_ERR_SUCCESS) {
					common::println_warn("Failed to reconnect to the server: ", mosquitto_strerror(ret), '!');
					continue;
				}

				// Subscribe to the topic
				if (int ret = mosquitto_subscribe(mosq, nullptr, "objects", 0); ret != MOSQ_ERR_SUCCESS) {
					common::println_warn("Failed to subscribe to the topic: ", mosquitto_strerror(ret), '!');
					continue;
				}

				common::println("OK");
			}

			return data;
		}
	}
};