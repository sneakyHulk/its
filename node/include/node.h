#pragma once

#include <oneapi/tbb/concurrent_queue.h>
#include <oneapi/tbb/concurrent_vector.h>

#include <chrono>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <utility>
using namespace std::chrono_literals;

template <typename Output>
class InputNode;

template <typename Input>
class OutputNode {
	template <typename T>
	friend class InputNode;

   protected:
	tbb::concurrent_queue<std::shared_ptr<Input const>> _input_queue;

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
				std::this_thread::sleep_for(1ms);
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
				std::this_thread::sleep_for(1ms);
			}
		}
	}
	virtual void output_function(std::shared_ptr<Input const> const&) = 0;
};

template <typename Input1, typename Input2>
class OutputNodePair {
	template <typename T>
	friend class InputNode;

   protected:
	std::atomic_flag _flag = ATOMIC_FLAG_INIT;
	std::vector<std::shared_ptr<Input1 const>> _input_vector;
	tbb::concurrent_queue<std::shared_ptr<Input2 const>> _input_queue;

	void input(std::shared_ptr<Input1 const>&& input) {
		while (_flag.test_and_set())
			;

		_input_vector.push_back(std::forward<decltype(input)>(input));

		_flag.clear();
	}
	void input(std::shared_ptr<Input2 const>&& input) { _input_queue.push(std::forward<decltype(input)>(input)); }

   public:
	virtual ~OutputNodePair() = default;
	[[noreturn]] virtual void operator()() {
		std::shared_ptr<Input2 const> item2;

		while (true) {
			while (!_input_queue.try_pop(item2)) {
				std::this_thread::sleep_for(1ms);
				std::this_thread::yield();
			}

			for (auto i = 0; i < _input_vector.size(); ++i) {
				if (output_function(*_input_vector[i], *item2)) {
					while (_flag.test_and_set())
						;

					_input_vector.erase(_input_vector.begin(), _input_vector.begin() + i + 1);

					_flag.clear();
				}
			}
		}
	}
	virtual bool output_function(Input1 const&, Input2 const&) = 0;
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
	template <typename dummy>
	void operator+=(OutputNodePair<Output, dummy>& node) {
		_output_connections.push_back([&node](std::shared_ptr<Output const>&& data) -> void { node.input(std::forward<decltype(data)>(data)); });
	}
	template <typename dummy>
	void operator+=(OutputNodePair<dummy, Output>& node) {
		_output_connections.push_back([&node](std::shared_ptr<Output const>&& data) -> void { node.input(std::forward<decltype(data)>(data)); });
	}
};

template <typename Output>
class ExternInputNode {
   protected:
	std::vector<std::function<void(std::shared_ptr<Output const>)>> _output_connections;

   public:
	virtual ~ExternInputNode() = default;

   protected:
	virtual void operator()() final {
		std::shared_ptr<Output const> output = std::make_shared<Output>(input_function());
		for (auto const& connection : _output_connections) connection(output);
	}

   public:
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

				for (auto const& connection : InputNode<Output>::_output_connections) {
					int i = 0;
					connection(output);
				}
			} else {
				std::this_thread::yield();
				std::this_thread::sleep_for(1ms);
			}
		}
	}

	virtual Output function(Input const&) = 0;
};