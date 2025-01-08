#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <vector>

#include "Node.h"
#include "Processor.h"
#include "Runner.h"
#include "RunnerSynchronous.h"
#include "RunnerSynchronousPair.h"

template <typename Input, typename Output>
class Processor;

/**
 * @class ProcessorSynchronous
 *
 * @attention This node has to be synchronously connected by calling synchronously_connect, to other pipeline nodes to receive input data.
 *
 * @tparam Input1 The first input type.
 * @tparam Input2 The second input type.
 * @tparam Output The output type.
 */
template <typename Input1, typename Input2, typename Output>
class ProcessorSynchronousPair : public Node {
	template <typename T>
	friend class Pusher;
	template <typename T1, typename T2>
	friend class Processor;
	template <typename T>
	friend class Runner;
	template <typename T>
	friend class RunnerSynchronous;
	template <typename T1, typename T2>
	friend class RunnerSynchronousPair;

	std::atomic<std::shared_ptr<Input1 const>> first = nullptr;
	std::atomic<std::shared_ptr<Input2 const>> second = nullptr;

	std::vector<std::function<void(Output const&)>> synchronous_functions;

	std::function<bool(Input1 const&, Input2 const&)> gate;

   protected:
	ProcessorSynchronousPair(std::function<bool(Input1 const&, Input2 const&)>&& gate) : gate(std::forward<decltype(gate)>(gate)) {}

   public:
	/**
	 * @brief Connects this ProcessorSynchronousPair to a Runner node for synchronous execution.
	 * @param node The Runner node to connect.
	 */
	[[maybe_unused]] void synchronously_connect(Runner<Output>& node) {
		synchronous_functions.push_back([&node](Output const& data) -> void { node.synchronous_call(std::forward<decltype(data)>(data)); });
	}
	/**
	 * @brief Connects this ProcessorSynchronousPair to a RunnerSynchronous node for synchronous execution.
	 * @param node The RunnerSynchronous node to connect.
	 */
	[[maybe_unused]] void synchronously_connect(RunnerSynchronous<Output>& node) {
		synchronous_functions.push_back([&node](Output const& data) -> void { node.synchronous_call(std::forward<decltype(data)>(data)); });
	}
	/**
	 * @brief Connects this ProcessorSynchronousPair to a RunnerSynchronousPair node for synchronous execution.
	 * @param node The RunnerSynchronousPair node to connect.
	 *
	 * @tparam dummy A dummy parameter that represents the other input template parameter of the connecting RunnerSynchronousPair node.
	 */
	template <typename dummy>
	[[maybe_unused]] void synchronously_connect(RunnerSynchronousPair<Output, dummy>& node) {
		synchronous_functions.push_back([&node](Output const& data) -> void { node.synchronous_call(std::forward<decltype(data)>(data)); });
	}
	/**
	 * @brief Connects this ProcessorSynchronousPair to a RunnerSynchronousPair node for synchronous execution.
	 * @param node The RunnerSynchronousPair node to connect.
	 *
	 * @tparam dummy A dummy parameter that represents the other input template parameter of the connecting RunnerSynchronousPair node.
	 */
	template <typename dummy>
	[[maybe_unused]] void synchronously_connect(RunnerSynchronousPair<dummy, Output>& node) {
		synchronous_functions.push_back([&node](Output const& data) -> void { node.synchronous_call(std::forward<decltype(data)>(data)); });
	}
	/**
	 * @brief Connects this ProcessorSynchronousPair to a Processor node for synchronous execution.
	 * @param node The Processor node to connect.
	 *
	 * @tparam dummy A dummy parameter that represents the output template parameter of the connecting Processor node.
	 *
	 * @return The connected Processor node to chain the connection description.
	 */
	template <typename dummy>
	[[maybe_unused]] Processor<Output, dummy>& synchronously_connect(Processor<Output, dummy>& node) {
		synchronous_functions.push_back([&node](Output const& data) -> void { node.synchronous_call(std::forward<decltype(data)>(data)); });

		return node;
	}
	/**
	 * @brief Connects this ProcessorSynchronousPair to another ProcessorSynchronousPair node for synchronous execution.
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
	 * @brief Connects this ProcessorSynchronousPair to another ProcessorSynchronousPair node for synchronous execution.
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
	 * @brief Does one iteration of this ProcessorSynchronousPair node.
	 */
	void synchronous_call(Input1 const& input) {
		std::atomic_store(&first, std::make_shared<Input1 const>(input));
		if (auto second_ptr = std::atomic_load(&second); second_ptr) {
			if (gate(input, *second_ptr)) {
				Output const output = process(input, *second_ptr);

				for (auto& connection : synchronous_functions) connection(output);
			}
		}
	}
	void synchronous_call(Input2 const& input) {
		std::atomic_store(&second, std::make_shared<Input2 const>(input));
		if (auto first_ptr = std::atomic_load(&first); first_ptr) {
			if (gate(*first_ptr, input)) {
				Output const output = process(*first_ptr, input);

				for (auto& connection : synchronous_functions) connection(output);
#
			}
		}
	}

	/**
	 * @brief Abstract method for presenting input data.
	 * @param data Input data to be presented.
	 */
	virtual Output process(Input1 const& data1, Input2 const& data2) = 0;
};