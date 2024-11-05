#include <oneapi/tbb/concurrent_queue.h>

#include <chrono>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <thread>
#include <utility>

#include "Object.h"
#include "common_output.h"

using namespace std::chrono_literals;

template <typename Input1, typename Input2>
class OutputNodePair {
	template <typename T>
	friend class InputNode;

   protected:
	tbb::concurrent_queue<std::shared_ptr<Input1 const>> _input_queue1;
	tbb::concurrent_queue<std::shared_ptr<Input2 const>> _input_queue2;

	void input(std::shared_ptr<Input1 const>&& input) { _input_queue1.push(std::forward<decltype(input)>(input)); }
	void input(std::shared_ptr<Input2 const>&& input) { _input_queue2.push(std::forward<decltype(input)>(input)); }

   public:
	virtual ~OutputNodePair() = default;
	[[noreturn]] virtual void operator()() {
		std::shared_ptr<Input1 const> item1;
		std::shared_ptr<Input2 const> item2;
		while (true) {
			if (_input_queue1.try_pop(item1)) {
				if (_input_queue2.try_pop(item2)) {
					run(*item1, *item2);
				} else {
					std::this_thread::yield();
				}
			} else {
				std::this_thread::yield();
			}
		}
	}
	virtual void run(Input1 const&, Input2 const&) = 0;
};

template <typename Output>
class InputNode {
   protected:
	std::vector<std::function<void(std::shared_ptr<Output const>)>> _output_connections;

   public:
	virtual ~InputNode() = default;
	[[noreturn]] virtual void operator()() {
		while (true) {
			std::shared_ptr<Output const> output = std::make_shared<Output>(input_function());

			for (auto const& connection : _output_connections) connection(output);
		}
	}

	virtual Output input_function() = 0;
	template <typename dummy>
	void operator+=(OutputNodePair<Output, dummy>& node) {
		_output_connections.push_back([&node](std::shared_ptr<Output const>&& data) -> void { node.input(std::forward<decltype(data)>(data)); });
	}
	template <typename dummy>
	void operator+=(OutputNodePair<dummy, Output>& node) {
		_output_connections.push_back([&node](std::shared_ptr<Output const>&& data) -> void { node.input(std::forward<decltype(data)>(data)); });
	}
};

class GenerateInts : public InputNode<int> {
	int input_function() final {
		std::this_thread::sleep_for(100ms);

		static int i = 0;

		return ++i;
	}
};

class GenerateFloats : public InputNode<float> {
	float input_function() final {
		static float f = 0;
		std::this_thread::sleep_for(200ms);

		return ++f;
	}
};

class IncomingIntFloat : public OutputNodePair<int, float> {
	void run(int const& item1, float const& item2) { common::println(item1, " - ", item2); }
};

int main() {
	GenerateFloats genf;
	GenerateInts geni;
	IncomingIntFloat inc;

	genf += inc;

	geni += inc;

	std::thread genf_thread(&GenerateFloats::operator(), &genf);
	std::thread geni_thread(&GenerateInts::operator(), &geni);

	inc();
}