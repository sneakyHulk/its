#pragma once

#include <oneapi/tbb/concurrent_queue.h>

#include <functional>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include "Node.h"
#include "ProcessorSynchronousPair.h"
#include "Runner.h"
#include "RunnerSynchronous.h"
#include "RunnerSynchronousPair.h"

template <typename Input1, typename Input2, typename Output>
class ProcessorSynchronousPair;

/**
 * @class Processor
 * @brief Template class for pipeline nodes that transform input data to output data.
 *
 * This class can receive input data from Pusher nodes and other Processor nodes, transforms their input data and forward the output data to other Processor nodes or Runner nodes.
 * This can be done synchronously or asynchronously and requires a prior connection.
 * In asynchronous procedures the input node fills a concurrent queue.
 * This queue is read from by the operating thread created by this class and the input can be processed.
 * In synchronous procedures the thread of the input node performs the processing.
 *
 * @attention When an asynchronous procedure is desired, operator()() has to be called first to start the thread.
 * @attention This node has to be synchronously connected by calling synchronously_connect or asynchronously connected by calling asynchronously_connect, to other pipeline nodes to receive input and forward output data.
 *
 * @tparam Input The input type.
 * @tparam Output The output type.
 */
template <typename Input, typename Output>
class Processor : public Node {
	template <typename T>
	friend class Pusher;
	template <typename T1, typename T2>
	friend class Processor;
	template <typename T1, typename T2, typename T3>
	friend class ProcessorSynchronousPair;
	template <typename T>
	friend class Runner;
	template <typename T>
	friend class RunnerSynchronous;
	template <typename T1, typename T2>
	friend class RunnerSynchronousPair;

	/**
	 * @brief Functions of other pipeline nodes that are executed synchronously after the execution of this Processor node.
	 *
	 * Functions run in the same thread, so there is no need for shared memory i.e. can use stack.
	 */
	std::vector<std::function<void(Output const&)>> synchronous_functions;
	std::vector<std::function<void(Output const&)>> synchronous_functions_once;

	/**
	 * @brief Functions of other pipeline nodes that are executed asynchronously after the execution of this Processor node.
	 */
	std::vector<std::function<void(std::shared_ptr<Output const>)>> asynchronous_functions;

	/**
	 * @brief Concurrent queue in which other pipeline nodes can input data asynchronously
	 */
	tbb::concurrent_bounded_queue<std::shared_ptr<Input const>> asynchronous_queue;

   protected:
	std::stop_token stop_token;

   public:
	/**
	 * @brief Starts the Processor node's processing loop in a separate thread.
	 * @attention All synchronous clients must be constructed before this function is called. If not, this is undefined behavior because the synchronous client has already been destroyed, and this thread could still call the functions of
	 * that destroyed synchronous client.
	 * @return The thread used for running the Processor node asynchronously.
	 */
	[[nodiscard("The thread would stop immediately!")]] [[maybe_unused]] std::jthread operator()() {
		return std::jthread([this](std::stop_token const& _stop_token) {
			stop_token = _stop_token;

			if (!stop_token.stop_requested()) {
				std::shared_ptr<Input const> item;
				asynchronous_queue.pop(item);

				synchronous_call_once(*item);
			}

			while (!stop_token.stop_requested()) {
				std::shared_ptr<Input const> item;
				asynchronous_queue.pop(item);

				synchronous_call(*item);
			}
		});
	}

	/**
	 * @brief Connects this Processor to a Runner node for synchronous execution.
	 * @param node The Runner node to connect.
	 */
	[[maybe_unused]] void synchronously_connect(Runner<Output>& node) {
		synchronous_functions.push_back([&node](Output const& data) -> void { node.synchronous_call(std::forward<decltype(data)>(data)); });
		synchronous_functions_once.push_back([&node](Output const& data) -> void { node.synchronous_call_once(std::forward<decltype(data)>(data)); });
	}
	/**
	 * @brief Connects this Processor to a RunnerSynchronous node for synchronous execution.
	 * @param node The RunnerSynchronous node to connect.
	 */
	[[maybe_unused]] void synchronously_connect(RunnerSynchronous<Output>& node) {
		synchronous_functions.push_back([&node](Output const& data) -> void { node.synchronous_call(std::forward<decltype(data)>(data)); });
		synchronous_functions_once.push_back([&node](Output const& data) -> void { node.synchronous_call_once(std::forward<decltype(data)>(data)); });
	}
	/**
	 * @brief Connects this Processor to a RunnerSynchronousPair node for synchronous execution.
	 * @param node The RunnerSynchronousPair node to connect.
	 *
	 * @tparam dummy A dummy parameter that represents the other input template parameter of the connecting RunnerSynchronousPair node.
	 */
	template <typename dummy>
	[[maybe_unused]] void synchronously_connect(RunnerSynchronousPair<Output, dummy>& node) {
		synchronous_functions.push_back([&node](Output const& data) -> void { node.synchronous_call(std::forward<decltype(data)>(data)); });
	}

	/**
	 * @brief Connects this Processor to a RunnerSynchronousPair node for synchronous execution.
	 * @param node The RunnerSynchronousPair node to connect.
	 *
	 * @tparam dummy A dummy parameter that represents the other input template parameter of the connecting RunnerSynchronousPair node.
	 */
	template <typename dummy>
	[[maybe_unused]] void synchronously_connect(RunnerSynchronousPair<dummy, Output>& node) {
		synchronous_functions.push_back([&node](Output const& data) -> void { node.synchronous_call(std::forward<decltype(data)>(data)); });
	}
	/**
	 * @brief Connects this Processor to another Processor node for synchronous execution.
	 * @param node The Processor node to connect.
	 *
	 * @tparam dummy A dummy parameter that represents the output template parameter of the connecting Processor node.
	 *
	 * @return The connected Processor node to chain the connection description.
	 */
	template <typename dummy>
	[[maybe_unused]] Processor<Output, dummy>& synchronously_connect(Processor<Output, dummy>& node) {
		synchronous_functions.push_back([&node](Output const& data) -> void { node.synchronous_call(std::forward<decltype(data)>(data)); });
		synchronous_functions_once.push_back([&node](Output const& data) -> void { node.synchronous_call_once(std::forward<decltype(data)>(data)); });

		return node;
	}

	/**
	 * @brief Connects this Processor to a Runner node for asynchronous execution.
	 * @param node The Runner node to connect.
	 */
	[[maybe_unused]] void asynchronously_connect(Runner<Output>& node) {
		asynchronous_functions.push_back([&node](std::shared_ptr<Output const>&& data) -> void { node.asynchronous_call(std::forward<decltype(data)>(data)); });
	}
	/**
	 * @brief Connects this Processor to another Processor node for asynchronous execution.
	 * @param node The Processor node to connect.
	 *
	 * @tparam dummy A dummy parameter that represents the output template parameter of the connecting Processor node.
	 *
	 * @return The connected Processor node to chain the connection description.
	 */
	template <typename dummy>
	[[maybe_unused]] Processor<Output, dummy>& asynchronously_connect(Processor<Output, dummy>& node) {
		asynchronous_functions.push_back([&node](std::shared_ptr<Output const>&& data) -> void { node.asynchronous_call(std::forward<decltype(data)>(data)); });

		return node;
	}

	/**
	 * @brief Connects this Processor to a ProcessorSynchronousPair node for synchronous execution.
	 * @param node The ProcessorSynchronousPair node to connect.
	 *
	 * @tparam dummy1 A dummy parameter that represents the output template parameter of the connecting ProcessorSynchronousPair node.
	 * @tparam dummy2 A dummy parameter that represents the output template parameter of the connecting ProcessorSynchronousPair node.
	 */
	template <typename dummy1, typename dummy2>
	[[maybe_unused]] void synchronously_connect(ProcessorSynchronousPair<Output, dummy1, dummy2>& node) {
		synchronous_functions.push_back([&node](Output const& data) -> void { node.synchronous_call(std::forward<decltype(data)>(data)); });
	}
	/**
	 * @brief Connects this Processor to a ProcessorSynchronousPair node for synchronous execution.
	 * @param node The ProcessorSynchronousPair node to connect.
	 *
	 * @tparam dummy1 A dummy parameter that represents the output template parameter of the connecting ProcessorSynchronousPair node.
	 * @tparam dummy2 A dummy parameter that represents the output template parameter of the connecting ProcessorSynchronousPair node.
	 */
	template <typename dummy1, typename dummy2>
	[[maybe_unused]] void synchronously_connect(ProcessorSynchronousPair<dummy1, Output, dummy2>& node) {
		synchronous_functions.push_back([&node](Output const& data) -> void { node.synchronous_call(std::forward<decltype(data)>(data)); });
	}

   private:
	/**
	 * @brief Does one iteration of this Processor node.
	 */
	void synchronous_call(Input const& input) {
		Output const output = process(input);

		if (!asynchronous_functions.empty()) {
			for (std::shared_ptr<Output const> output_ptr = std::make_shared<Output const>(output); auto& connection : asynchronous_functions) connection(output_ptr);
		}
		for (auto& connection : synchronous_functions) connection(output);
	}
	/**
	 * @brief Does one iteration of this Processor node.
	 */
	void synchronous_call_once(Input const& input) {
		Output const output = process_once(input);

		if (!asynchronous_functions.empty()) {
			for (std::shared_ptr<Output const> output_ptr = std::make_shared<Output const>(output); auto& connection : asynchronous_functions) connection(output_ptr);
		}
		for (auto& connection : synchronous_functions_once) connection(output);
	}

	/**
	 * @brief Puts the input data in the concurrent queue to be read from this Processor node's thread asynchronously.
	 */
	void asynchronous_call(std::shared_ptr<Input const> input) {
		if (!asynchronous_queue.try_push(std::forward<decltype(input)>(input))) std::cout << typeid(*this).name() << " skipped!" << std::endl;
	}

	/**
	 * @brief Abstract method for transforming input data to output data.
	 * @param data Input data to be transformed.
	 * @return Transformed output data.
	 */
	virtual Output process(Input const& data) = 0;
	/**
	 * @brief Optional method for transforming input data to output data.
	 * @param data Input data to be transformed.
	 * @return Transformed output data.
	 */
	virtual Output process_once(Input const& data) { return process(std::forward<decltype(data)>(data)); }
};