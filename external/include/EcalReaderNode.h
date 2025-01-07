#pragma once

#include <ecal/ecal.h>

#include <filesystem>
#include <opencv2/opencv.hpp>

#include "ImageData.h"
#include "Pusher.h"
#include "common_output.h"
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

		index = _subscriber_map.begin();
	}

   private:
	std::map<std::string, SubscriberConfig> _subscriber_map;
	decltype(_subscriber_map)::iterator index;

	ImageData push() final {
		for (;; ++index != _subscriber_map.end() ? index : index = _subscriber_map.begin()) {
			std::string image_str;
			long long int timestamp;
			if (auto const n = index->second.subscriber.ReceiveBuffer(image_str, &timestamp, 1000); !n) continue;

			providentia::StreamingMessage msg;
			msg.mutable_image()->set_data(image_str.data(), image_str.length());

			ImageData ret;
			ret.source = index->first;
			ret.timestamp = timestamp;
			ret.image = cv::Mat(1200, 1920, CV_8UC3, (void*)msg.mutable_image()->data().c_str(), cv::Mat::AUTO_STEP).clone();

			return ret;
		}
	}
};