#pragma once

#include <memory>
#include <atomic>

#include "Node.h"

/**
 * @class RunnerSynchronous
 * @brief Template class for pipeline nodes that present input data or do stuff outside the pipeline.
 *
 * This class can receive input data from Pusher nodes and other Processor nodes and present the input data i.e. make data available outside the pipeline.
 * This can be done synchronously and requires a prior connection.
 * In this synchronous procedures the thread of the input node performs the presenting.
 *
 * @attention This node has to be synchronously connected by calling synchronously_connect, to other pipeline nodes to receive input data.
 *
 * @tparam Input The input type.
 */
template <typename Input1, typename Input2>
class RunnerSynchronousPair : public Node {
	template <typename T>
	friend class Pusher;
	template <typename T1, typename T2>
	friend class Processor;

	std::shared_ptr<Input1 const> first = nullptr;
	std::shared_ptr<Input2 const> second = nullptr;

public:
	~RunnerSynchronousPair() override = default;

private:
	/**
	 * @brief Does one iteration of this RunnerSynchronous node.
	 */
	void synchronous_call(Input1 const& input) {  if(auto second_ptr = std::atomic_load(&second); second_ptr) run(input, *second_ptr); std::atomic_store(&first, std::make_shared<Input1 const>(input)); }
	void synchronous_call(Input2 const& input) {   if(auto first_ptr = std::atomic_load(&second); first_ptr) run(*first_ptr, input); std::atomic_store(&second, std::make_shared<Input2 const>(input));}

	/**
	 * @brief Abstract method for presenting input data.
	 * @param data Input data to be presented.
	 */
	virtual void run(Input1 const& data1, Input2 const& data2) = 0;
};