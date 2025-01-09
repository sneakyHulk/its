#pragma once

#include <functional>
#include <memory>
#include <thread>
#include <vector>

#include "Node.h"
#include "ProcessorSynchronousPair.h"
#include "Runner.h"
#include "RunnerSynchronous.h"
#include "RunnerSynchronousPair.h"

template <typename Input, typename Output>
class Processor;
template <typename Input1, typename Input2, typename Output>
class ProcessorSynchronousPair;

/**
 * @class Pusher
 * @brief Template class for pipeline nodes that generate and forward output data.
 *
 * This class can generate output data and forward the output data to Processor nodes or Runner nodes.
 * This can be done asynchronously and requires a prior connection.
 * In asynchronous procedures this node fills a concurrent queue.
 * This queue is read from by the receiving nodes the operating thread created by this class and the input can be processed.
 *
 * @attention When an asynchronous procedure is desired, operator()() has to be called first to start the thread.
 * @attention This node has to be synchronously connected by calling synchronously_connect or asynchronously connected by calling asynchronously_connect, to other pipeline nodes to forward output data.
 *
 * @tparam Output The output type.
 */
template <typename Output>
class Pusher : public Node {
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
	 * @brief Functions of other pipeline nodes that are executed synchronously after the execution of this Pusher node.
	 *
	 * Functions run in the same thread, so there is no need for shared memory i.e. can use stack.
	 */
	std::vector<std::function<void(Output const&)>> synchronous_functions;
	std::vector<std::function<void(Output const&)>> synchronous_functions_once;

	/**
	 * @brief Functions of other pipeline nodes that are executed asynchronously after the execution of this Pusher node.
	 */
	std::vector<std::function<void(std::shared_ptr<Output const>)>> asynchronous_functions;

   protected:
	std::stop_token stop_token;

   public:
	/**
	 * @brief Starts the Pusher node's processing loop in a separate thread.
	 * @attention All synchronous clients must be constructed before this function is called. If not, this is undefined behavior because the synchronous client has already been destroyed, and this thread could still call the functions of
	 * that destroyed synchronous client.
	 * @return The thread used for running the Pusher node asynchronously.
	 */
	[[nodiscard("The thread would stop immediately!")]] std::jthread operator()() {
		return std::jthread([this](std::stop_token const& _stop_token) {
			stop_token = _stop_token;

			if (!stop_token.stop_requested()) {
				synchronous_call_once();
			}

			while (!stop_token.stop_requested()) {
				synchronous_call();
			}
		});
	}

	/**
	 * @brief Connects this Pusher to a Runner node for synchronous execution.
	 * @param node The Runner node to connect.
	 */
	[[maybe_unused]] void synchronously_connect(Runner<Output>& node) {
		synchronous_functions.push_back([&node](Output const& data) -> void { node.synchronous_call(std::forward<decltype(data)>(data)); });
		synchronous_functions_once.push_back([&node](Output const& data) -> void { node.synchronous_call_once(std::forward<decltype(data)>(data)); });
	}
	/**
	 * @brief Connects this Pusher to a RunnerSynchronous node for synchronous execution.
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
	 * @brief Connects this Pusher to a Processor node for synchronous execution.
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
	 * @brief Connects this Pusher to a ProcessorSynchronousPair node for synchronous execution.
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
	 * @brief Connects this Pusher to a ProcessorSynchronousPair node for synchronous execution.
	 * @param node The ProcessorSynchronousPair node to connect.
	 *
	 * @tparam dummy1 A dummy parameter that represents the output template parameter of the connecting ProcessorSynchronousPair node.
	 * @tparam dummy2 A dummy parameter that represents the output template parameter of the connecting ProcessorSynchronousPair node.
	 */
	template <typename dummy1, typename dummy2>
	[[maybe_unused]] void synchronously_connect(ProcessorSynchronousPair<dummy1, Output, dummy2>& node) {
		synchronous_functions.push_back([&node](Output const& data) -> void { node.synchronous_call(std::forward<decltype(data)>(data)); });
	}

	/**
	 * @brief Connects this Pusher to a Runner node for asynchronous execution.
	 * @param node The Runner node to connect.
	 */
	[[maybe_unused]] void asynchronously_connect(Runner<Output>& node) {
		asynchronous_functions.push_back([&node](std::shared_ptr<Output const>&& data) -> void { node.asynchronous_call(std::forward<decltype(data)>(data)); });
	}
	/**
	 * @brief Connects this Pusher to a Processor node for asynchronous execution.
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

   private:
	/**
	 * @brief Does one iteration of this Pusher node.
	 */
	void synchronous_call() {
		Output const output = push();

		if (!asynchronous_functions.empty()) {
			for (std::shared_ptr<Output const> output_ptr = std::make_shared<Output const>(output); auto& connection : asynchronous_functions) connection(output_ptr);
		}
		for (auto& connection : synchronous_functions) connection(output);
	}
	/**
	 * @brief Does one iteration of this Pusher node.
	 */
	void synchronous_call_once() {
		Output const output = push_once();

		if (!asynchronous_functions.empty()) {
			for (std::shared_ptr<Output const> output_ptr = std::make_shared<Output const>(output); auto& connection : asynchronous_functions) connection(output_ptr);
		}
		for (auto& connection : synchronous_functions_once) connection(output);
	}

	/**
	 * @brief Abstract method for generating output data.
	 * @return Generated output data.
	 */
	virtual Output push() = 0;
	/**
	 * @brief Optional method for generating output data once.
	 * @return Generated output data.
	 */
	virtual Output push_once() { return push(); }
};
