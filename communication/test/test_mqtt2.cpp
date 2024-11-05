#include <chrono>
#include <thread>

#include "data_communication_node.h"
using namespace std::chrono_literals;

class GenerateCompactObjects : public InputNode<CompactObjects> {
	CompactObjects input_function() final {
		std::this_thread::sleep_for(50ms);
		std::uint64_t now = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now()).time_since_epoch().count();
		return CompactObjects{now, {}};
	}
};

class PrintCompactObjects : public Runner<CompactObjects> {
	void run(CompactObjects const& data) final {
		common::println(data.timestamp, " took ", std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now() - std::chrono::time_point<std::chrono::system_clock>(std::chrono::nanoseconds(data.timestamp))));
	}
};

int main(int argc, char** argv) {
	DataStreamMQTT data_stream;
	GenerateCompactObjects generate;

	generate += data_stream;

	DataInputMQTT data_input;
	PrintCompactObjects print;

	data_input += print;

	std::thread data_stream_thread(&DataStreamMQTT::operator(), &data_stream);
	std::thread data_input_thread(&DataInputMQTT::operator(), &data_input);
	std::thread print_thread(&PrintCompactObjects::operator(), &print);

	generate();
}