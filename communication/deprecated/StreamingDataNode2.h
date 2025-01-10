#pragma once

#include <mqtt/client.h>

#include "CompactObject.h"
#include "Runner.h"

/**
 * @class StreamingDataNode
 * @brief A class responsible for handling streaming data and publishing it to an MQTT broker.
 */
class StreamingDataNode : public Runner<CompactObjects> {
	mqtt::client cli;

   public:
	explicit StreamingDataNode();

	void run(CompactObjects const &data) final;
};