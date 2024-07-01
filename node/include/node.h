#pragma once

#include <oneapi/tbb/concurrent_queue.h>

#include <memory>
#include <ranges>
#include <stdexcept>
#include <utility>

template <typename Output>
class InputNode;

template <typename Input>
class OutputNode {
   protected:
	tbb::concurrent_queue<std::shared_ptr<Input const>> _input_queue;

	template <typename T>
	friend class InputNode;

	void input(std::shared_ptr<Input const>&& input) { _input_queue.push(std::forward<decltype(input)>(input)); }

   public:
	virtual ~OutputNode() = default;
	[[noreturn]] virtual void operator()() {
		std::shared_ptr<Input const> item;
		while (true) {
			if (_input_queue.try_pop(item)) {
				output_function(*item);
			} else {
				std::this_thread::yield();
			}
		}
	}
	virtual void output_function(Input const&) = 0;
};

template <typename Input>
class OutputPtrNode : public OutputNode<Input> {
	void output_function(Input const&) final { throw std::logic_error("unreachable code"); }

   public:
	[[noreturn]] void operator()() override {
		std::shared_ptr<Input const> item;
		while (true) {
			if (OutputNode<Input>::_input_queue.try_pop(item)) {
				output_function(item);
			} else {
				std::this_thread::yield();
			}
		}
	}
	virtual void output_function(std::shared_ptr<Input const> const&) = 0;
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
	void operator+=(OutputNode<Output>& node) { _output_connections.push_back(std::bind(&OutputNode<Output>::input, &node, std::placeholders::_1)); }
};

template <typename Input, typename Output>
class InputOutputNode : public InputNode<Output>, public OutputNode<Input> {
	Output input_function() final { throw std::logic_error("unreachable code"); };
	void output_function(Input const&) final { throw std::logic_error("unreachable code"); };

   public:
	[[noreturn]] void operator()() final {
		while (true) {
			std::shared_ptr<Input const> item;
			if (OutputNode<Input>::_input_queue.try_pop(item)) {
				std::shared_ptr<Output const> output = std::make_shared<Output>(function(*item));

				for (auto const& connection : InputNode<Output>::_output_connections) connection(output);
			} else {
				std::this_thread::yield();
			}
		}
	}

	virtual Output function(Input const&) = 0;
};