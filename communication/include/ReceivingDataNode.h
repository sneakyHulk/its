#pragma once

#include <mqtt/client.h>

#include "CompactObject.h"
#include "Pusher.h"

/**
 * @class ReceivingDataNode
 * @brief A class responsible for subscribing to an MQTT topic and deserializing received data.
 */
class ReceivingDataNode : public Pusher<CompactObjects> {
	mqtt::client cli;

public:
	ReceivingDataNode();

 CompactObjects push() final;
};