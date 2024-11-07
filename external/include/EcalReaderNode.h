#pragma once

#include <ecal/ecal.h>

#include <filesystem>
#include <opencv2/opencv.hpp>

#include "ImageData.h"
#include "common_output.h"
#include "node.h"
#include "streaming_message.pb.h"

class EcalReaderNode : public Pusher<ImageData> {
	struct SubscriberConfig {
		eCAL::CSubscriber subscriber;
	};

   public:
	struct TopicNameConfig {
		std::string topic_name;
	};

	explicit EcalReaderNode(std::map<std::string, TopicNameConfig>&& config) {
		for (auto const& [camera_name, topic_name_config] : config) {
			_subscriber_map[camera_name].subscriber.Create(topic_name_config.topic_name);
		}
	}

   private:
	std::map<std::string, SubscriberConfig> _subscriber_map;

	ImageData push() final {
		for (auto& [camera_name, subscriber] : _subscriber_map) {
			subscriber.subscriber.
		}

		return ImageData;
	}
};