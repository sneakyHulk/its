#include <chrono>
#include <thread>

#include "Pusher.h"
#include "ReceivingDataNode.h"
#include "Runner.h"
#include "StreamingDataNode.h"
#include "common_output.h"

using namespace std::chrono_literals;

class GenerateCompactObjects : public Pusher<CompactObjects> {
	CompactObjects push() final {
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
	StreamingDataNode data_stream;
	GenerateCompactObjects generate;

	generate.synchronously_connect(data_stream);

	ReceivingDataNode data_input;
	PrintCompactObjects print;

	data_input.synchronously_connect(print);

	auto generate_thread = generate();
	auto data_input_thread = data_input();

	std::this_thread::sleep_for(100s);
}