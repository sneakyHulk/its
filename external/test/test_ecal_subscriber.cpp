#include <ecal/ecal.h>
#include <ecal/msg/string/subscriber.h>

#include <iostream>
#include <thread>

int main(int argc, char** argv) {
	// Initialize eCAL
	eCAL::Initialize(argc, argv, "Hello World Subscriber");

	// Create a subscriber that listenes on the "hello_world_topic"
	eCAL::string::CSubscriber<std::string> subscriber("hello_world_topic");

	// Just don't exit
	while (eCAL::Ok()) {
		std::string message;
		long long int timestamp;
		if (auto const n = subscriber.ReceiveBuffer(message, &timestamp, 1000); !n) continue;

		std::cout << "Received Message: " << message << std::endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}

	// finalize eCAL API
	eCAL::Finalize();
}
