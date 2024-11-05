#pragma once

#include <oneapi/tbb/concurrent_queue.h>

#include <chrono>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <utility>

#include "common_output.h"

extern std::function<void()> clean_up;

using namespace std::chrono_literals;

template <typename Input>
class Runner {
	template <typename T>
	friend class Pusher;

	template <typename T1, typename T2>
	friend class Processor;

	tbb::concurrent_bounded_queue<std::shared_ptr<Input const>> _input_queue;

	void input(std::shared_ptr<Input const>&& input) {
		if (!_input_queue.try_push(std::forward<decltype(input)>(input))) common::println(typeid(*this).name(), " skipped!");
	}

	std::thread thread;
	[[noreturn]] void thread_function() {
		try {
			while (true) {
				std::shared_ptr<Input const> item;
				_input_queue.pop(item);

				run(*item);
			}
		} catch (...) {
			clean_up();
		}
	}

   protected:
	Runner() { _input_queue.set_capacity(100); }
	virtual ~Runner() = default;

   public:
	void operator()() { thread = std::thread(&Runner<Input>::thread_function, this); }

   private:
	virtual void run(Input const&) = 0;
};

// template <typename Input>
// class OutputPtrNode : public OutputNode<Input> {
//	void output_function(Input const&) final { throw std::logic_error("unreachable code"); }
//
//    public:
//	[[noreturn]] virtual void operator()() {
//		try {
//			while (true) {
//				std::shared_ptr<Input const> item;
//				OutputNode<Input>::_input_queue.pop(item);
//				output_function(item);
//			}
//		} catch (...) {
//			current_exception = std::current_exception();
//			std::exit(1);
//		}
//	}
//	virtual void output_function(std::shared_ptr<Input const> const&) = 0;
// };

// template <typename Input1, typename Input2>  // dont work
// class OutputNodePair : public OutputNode<Input2> {
//	void output_function(Input2 const&) final { throw std::logic_error("unreachable code"); }
//
//	template <typename T>
//	friend class InputNode;
//
//    protected:
//	std::atomic_flag _flag = ATOMIC_FLAG_INIT;
//	std::vector<std::shared_ptr<Input1 const>> _input_vector;
//
//	using OutputNode<Input2>::input;  // name hiding see https://bastian.rieck.me/blog/2016/name_hiding_cxx/
//	void input(std::shared_ptr<Input1 const>&& input) {
//		while (_flag.test_and_set())
//			;
//
//		_input_vector.push_back(std::forward<decltype(input)>(input));
//
//		_flag.clear();
//	}
//
//    public:
//	virtual ~OutputNodePair() = default;
//	[[noreturn]] void operator()() final {
//		try {
//			while (true) {
//				std::shared_ptr<Input2 const> item2;
//				OutputNode<Input2>::_input_queue.pop(item2);
//
//				for (auto i = 0; i < _input_vector.size(); ++i) {
//					if (output_function(*_input_vector[i], *item2)) {
//						while (_flag.test_and_set())
//							;
//
//						_input_vector.erase(_input_vector.begin(), _input_vector.begin() + i + 1);
//
//						_flag.clear();
//					}
//				}
//			}
//		} catch (...) {
//			current_exception = std::current_exception();
//			std::exit(1);
//		}
//	}
//	virtual bool output_function(Input1 const&, Input2 const&) = 0;
// };

template <typename Input, typename Output>
class Processor;

template <typename Output>
class Pusher {
	template <typename T>
	friend class Runner;

	template <typename T1, typename T2>
	friend class Processor;

	std::vector<std::function<void(std::shared_ptr<Output const>)>> _output_connections;

	std::thread thread;
	[[noreturn]] void thread_function() {
		try {
			while (true) {
				std::shared_ptr<Output const> output = std::make_shared<Output>(push());

				for (auto const& connection : _output_connections) connection(output);
			}
		} catch (...) {
			clean_up();
		}
	}

   protected:
	Pusher() = default;
	virtual ~Pusher() = default;

   public:
	void operator()() { thread = std::thread(&Pusher<Output>::thread_function, this); }

	void operator+=(Runner<Output>& node) {
		_output_connections.push_back([&node](std::shared_ptr<Output const>&& data) -> void { node.input(std::forward<decltype(data)>(data)); });
	}

	template <typename dummy>
	void operator+=(Processor<Output, dummy>& node) {
		_output_connections.push_back([&node](std::shared_ptr<Output const>&& data) -> void { node.input(std::forward<decltype(data)>(data)); });
	}

   private:
	virtual Output push() = 0;

	// void operator+=(OutputNode<Output>& node) {
	//	_output_connections.push_back([&node](std::shared_ptr<Output const>&& data) -> void { node.input(std::forward<decltype(data)>(data)); });
	// }
	// template <typename dummy>
	// void operator+=(OutputNodePair<Output, dummy>& node) {
	//	_output_connections.push_back([&node](std::shared_ptr<Output const>&& data) -> void { node.input(std::forward<decltype(data)>(data)); });
	// }
	// template <typename dummy>
	// void operator+=(OutputNodePair<dummy, Output>& node) {
	//	_output_connections.push_back([&node](std::shared_ptr<Output const>&& data) -> void { node.input(std::forward<decltype(data)>(data)); });
	// }
};

// template <typename Output>
// class InputNodeExternalTrigger : public InputNode<Output> {
//    protected:
//	std::vector<std::function<void(std::shared_ptr<Output const>)>> _output_connections;
//
//    public:
//	virtual ~InputNodeExternalTrigger() = default;
//	void operator()() final {}
//
//	void input_function_external_trigger() {
//		std::shared_ptr<Output const> output = std::make_shared<Output>(InputNode<Output>::input_function());
//
//		for (auto const& connection : _output_connections) connection(output);
//	}
// };

template <typename Input, typename Output>
class Processor {
	template <typename T>
	friend class Runner;

	template <typename T>
	friend class Pusher;

	std::vector<std::function<void(std::shared_ptr<Output const>)>> _output_connections;

	tbb::concurrent_bounded_queue<std::shared_ptr<Input const>> _input_queue;
	void input(std::shared_ptr<Input const>&& input) {
		if (!_input_queue.try_push(std::forward<decltype(input)>(input))) common::println(typeid(*this).name(), " skipped!");
	}

	std::thread thread;
	[[noreturn]] void thread_function() {
		try {
			while (true) {
				std::shared_ptr<Input const> item;
				_input_queue.pop(item);

				std::shared_ptr<Output const> output = std::make_shared<Output>(process(*item));

				for (auto const& connection : _output_connections) connection(output);
			}
		} catch (...) {
			clean_up();
		}
	}

   protected:
	Processor() = default;
	virtual ~Processor() {}

   public:
	void operator()() { thread = std::thread(&Processor<Input, Output>::thread_function, this); }

	void operator+=(Runner<Output>& node) {
		_output_connections.push_back([&node](std::shared_ptr<Output const>&& data) -> void { node.input(std::forward<decltype(data)>(data)); });
	}

	template <typename dummy>
	void operator+=(Processor<Output, dummy>& node) {
		_output_connections.push_back([&node](std::shared_ptr<Output const>&& data) -> void { node.input(std::forward<decltype(data)>(data)); });
	}

   private:
	virtual Output process(Input const&) = 0;
};

// template <typename Input, typename Output>
// class InputOutputNodeExternalTrigger : public InputNode<Output>, public OutputNode<Input> {
//	Output input_function() final { throw std::logic_error("unreachable code"); };
//	void output_function(Input const&) final { throw std::logic_error("unreachable code"); };
//
//	void operator()() final {}
//
//	virtual Output function(Input const&) = 0;
//
//	void function_external_trigger(Input const& data) {
//		std::shared_ptr<Output const> output = std::make_shared<Output>(function(data));
//
//		for (auto const& connection : InputNode<Output>::_output_connections) connection(output);
//	}
// };
