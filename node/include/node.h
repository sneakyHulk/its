#pragma once

#include <oneapi/tbb/concurrent_queue.h>

#include <chrono>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <utility>

#include "common_exception.h"
#include "common_output.h"

class Node {
   public:
	virtual void name() { std::cout << typeid(*this).name() << std::endl; }

	static void run() {}

   private:
};

template <typename Input, typename Output>
class Processor;

template <typename Input>
class Runner;

template <typename Output>
class Pusher : public Node {
	template <typename T1, typename T2>
	friend class Processor;

	template <typename T>
	friend class Runner;

	std::thread thread;

	// synchronous do not need shared memory i.e. can use stack
	std::vector<std::function<void(Output const&)>> synchronous_functions;
	std::vector<std::function<void(Output const&)>> synchronous_functions_once;
	std::vector<std::function<void(std::shared_ptr<Output const>)>> asynchronous_functions;

   public:
	void operator()() {
		thread = std::thread([this] [[noreturn]] () {
			synchronous_call_once();

			while (true) {
				synchronous_call();
			}
		});
	}

	void synchronously_connect(Runner<Output>& node) {
		synchronous_functions.push_back([&node](Output const& data) -> void { node.synchronous_call(std::forward<decltype(data)>(data)); });
		synchronous_functions_once.push_back([&node](Output const& data) -> void { node.synchronous_call_once(std::forward<decltype(data)>(data)); });
	}
	template <typename dummy>
	Processor<Output, dummy>& synchronously_connect(Processor<Output, dummy>& node) {
		synchronous_functions.push_back([&node](Output const& data) -> void { node.synchronous_call(std::forward<decltype(data)>(data)); });
		synchronous_functions_once.push_back([&node](Output const& data) -> void { node.synchronous_call_once(std::forward<decltype(data)>(data)); });

		return node;
	}

	void asynchronously_connect(Runner<Output>& node) {
		asynchronous_functions.push_back([&node](std::shared_ptr<Output const>&& data) -> void { node.asynchronous_call(std::forward<decltype(data)>(data)); });
	}
	template <typename dummy>
	Processor<Output, dummy>& asynchronously_connect(Processor<Output, dummy>& node) {
		asynchronous_functions.push_back([&node](std::shared_ptr<Output const>&& data) -> void { node.asynchronous_call(std::forward<decltype(data)>(data)); });

		return node;
	}

   private:
	void synchronous_call() {
		Output const output = push();

		if (!asynchronous_functions.empty()) {
			for (std::shared_ptr<Output const> output_ptr = std::make_shared<Output const>(output); auto& connection : asynchronous_functions) connection(output_ptr);
		}
		for (auto& connection : synchronous_functions) connection(output);
	}

	void synchronous_call_once() {
		Output const output = push_once();

		if (!asynchronous_functions.empty()) {
			for (std::shared_ptr<Output const> output_ptr = std::make_shared<Output const>(output); auto& connection : asynchronous_functions) connection(output_ptr);
		}
		for (auto& connection : synchronous_functions_once) connection(output);
	}

	virtual Output push() = 0;
	virtual Output push_once() { return push(); }
};

template <typename Input>
class Runner : public Node {
	template <typename T>
	friend class Pusher;

	template <typename T1, typename T2>
	friend class Processor;

	std::thread thread;

	tbb::concurrent_bounded_queue<std::shared_ptr<Input const>> asynchronous_queue;

   public:
	void operator()() {
		thread = std::thread([this] [[noreturn]] () {
			{
				std::shared_ptr<Input const> item;
				asynchronous_queue.pop(item);

				synchronous_call_once(*item);
			}

			while (true) {
				std::shared_ptr<Input const> item;
				asynchronous_queue.pop(item);

				synchronous_call(*item);
			}
		});
	}

   private:
	void synchronous_call(Input const& input) { run(input); }
	void synchronous_call_once(Input const& input) { run_once(input); }
	void asynchronous_call(std::shared_ptr<Input const> input) {
		if (!asynchronous_queue.try_push(std::forward<decltype(input)>(input))) std::cout << typeid(*this).name() << " skipped!" << std::endl;
	}

	virtual void run(Input const& data) = 0;
	virtual void run_once(Input const& data) { run(std::forward<decltype(data)>(data)); }
};
template <typename Input, typename Output>
class Processor : public Node {
	template <typename T>
	friend class Pusher;

	template <typename T>
	friend class Runner;

	std::thread thread;

	std::vector<std::function<void(Output const&)>> synchronous_functions;
	std::vector<std::function<void(Output const&)>> synchronous_functions_once;
	std::vector<std::function<void(std::shared_ptr<Output const>)>> asynchronous_functions;

	tbb::concurrent_bounded_queue<std::shared_ptr<Input const>> asynchronous_queue;

   public:
	void operator()() {
		thread = std::thread([this] [[noreturn]] () {
			{
				std::shared_ptr<Input const> item;
				asynchronous_queue.pop(item);

				synchronous_call_once(*item);
			}

			while (true) {
				std::shared_ptr<Input const> item;
				asynchronous_queue.pop(item);

				synchronous_call(item);
			}
		});
	}

	void synchronously_connect(Runner<Output>& node) {
		synchronous_functions.push_back([&node](Output const& data) -> void { node.synchronous_call(std::forward<decltype(data)>(data)); });
		synchronous_functions_once.push_back([&node](Output const& data) -> void { node.synchronous_call_once(std::forward<decltype(data)>(data)); });
	}
	template <typename dummy>
	Processor<Output, dummy>& synchronously_connect(Processor<Output, dummy>& node) {
		synchronous_functions.push_back([&node](Output const& data) -> void { node.synchronous_call(std::forward<decltype(data)>(data)); });
		synchronous_functions_once.push_back([&node](Output const& data) -> void { node.synchronous_call_once(std::forward<decltype(data)>(data)); });

		return node;
	}

	void asynchronously_connect(Runner<Output>& node) {
		asynchronous_functions.push_back([&node](std::shared_ptr<Output const>&& data) -> void { node.asynchronous_call(std::forward<decltype(data)>(data)); });
	}
	template <typename dummy>
	Processor<Output, dummy>& asynchronously_connect(Processor<Output, dummy>& node) {
		asynchronous_functions.push_back([&node](std::shared_ptr<Output const>&& data) -> void { node.asynchronous_call(std::forward<decltype(data)>(data)); });
	}

   private:
	void synchronous_call(Input const& input) {
		Output const output = process(input);

		if (!asynchronous_functions.empty()) {
			for (std::shared_ptr<Output const> output_ptr = std::make_shared<Output const>(output); auto& connection : asynchronous_functions) connection(output_ptr);
		}
		for (auto& connection : synchronous_functions) connection(output);
	}
	void synchronous_call_once(Input const& input) {
		Output const output = process_once(input);

		if (!asynchronous_functions.empty()) {
			for (std::shared_ptr<Output const> output_ptr = std::make_shared<Output const>(output); auto& connection : asynchronous_functions) connection(output_ptr);
		}
		for (auto& connection : synchronous_functions_once) connection(output);
	}
	void asynchronous_call(std::shared_ptr<Input const> input) {
		if (!asynchronous_queue.try_push(std::forward<decltype(input)>(input))) std::cout << typeid(*this).name() << " skipped!" << std::endl;
	}

	virtual Output process(Input const& data) = 0;
	virtual Output process_once(Input const& data) { return process(std::forward<decltype(data)>(data)); }
};

/*extern std::function<void()> clean_up;

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

        std::unreachable();
    }

   protected:
    Runner() { _input_queue.set_capacity(100); }
    virtual ~Runner() noexcept(false) {}

   public:
    void operator()() { thread = std::thread(&Runner<Input>::thread_function, this); }

   private:
    virtual void run(Input const&) = 0;
};

template <typename Input>
class RunnerPtr {
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

                run(item);
            }
        } catch (...) {
            clean_up();
        }

        std::unreachable();
    }

   protected:
    RunnerPtr() { _input_queue.set_capacity(100); }
    virtual ~RunnerPtr() noexcept(false) {}

   public:
    void operator()() { thread = std::thread(&RunnerPtr<Input>::thread_function, this); }

   private:
    virtual void run(std::shared_ptr<Input const> const&) = 0;
};

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

        std::unreachable();
    }

   protected:
    Pusher() = default;
    virtual ~Pusher() noexcept(false) {}

   public:
    void operator()() { thread = std::thread(&Pusher<Output>::thread_function, this); }

    void operator+=(Runner<Output>& node) {
        _output_connections.push_back([&node](std::shared_ptr<Output const>&& data) -> void { node.input(std::forward<decltype(data)>(data)); });
    }

    void operator+=(RunnerPtr<Output>& node) {
        _output_connections.push_back([&node](std::shared_ptr<Output const>&& data) -> void { node.input(std::forward<decltype(data)>(data)); });
    }

    template <typename dummy>
    void operator+=(Processor<Output, dummy>& node) {
        _output_connections.push_back([&node](std::shared_ptr<Output const>&& data) -> void { node.input(std::forward<decltype(data)>(data)); });
    }

   private:
    virtual Output push() = 0;
};

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

        std::unreachable();
    }

   protected:
    Processor() = default;
    virtual ~Processor() noexcept(false) {}

   public:
    void operator()() { thread = std::thread(&Processor<Input, Output>::thread_function, this); }

    void operator+=(Runner<Output>& node) {
        _output_connections.push_back([&node](std::shared_ptr<Output const>&& data) -> void { node.input(std::forward<decltype(data)>(data)); });
    }

    void operator+=(RunnerPtr<Output>& node) {
        _output_connections.push_back([&node](std::shared_ptr<Output const>&& data) -> void { node.input(std::forward<decltype(data)>(data)); });
    }

    template <typename dummy>
    void operator+=(Processor<Output, dummy>& node) {
        _output_connections.push_back([&node](std::shared_ptr<Output const>&& data) -> void { node.input(std::forward<decltype(data)>(data)); });
    }

   private:
    virtual Output process(Input const&) = 0;
};*/