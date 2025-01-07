#pragma once

#include <memory>

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
template <typename Input>
class RunnerSynchronous : public Node {
	template <typename T>
	friend class Pusher;
	template <typename T1, typename T2>
	friend class Processor;
	template <typename T1, typename T2, typename T3>
	friend class ProcessorSynchronousPair;

   private:
	/**
	 * @brief Does one iteration of this RunnerSynchronous node.
	 */
	void synchronous_call(Input const& input) { run(input); }
	/**
	 * @brief Does one iteration of this RunnerSynchronous node.
	 */
	void synchronous_call_once(Input const& input) { run_once(input); }

	/**
	 * @brief Abstract method for presenting input data.
	 * @param data Input data to be presented.
	 */
	virtual void run(Input const& data) = 0;
	/**
	 * @brief Optional method for presenting input data once.
	 * @param data Input data to be presented.
	 */
	virtual void run_once(Input const& data) { run(std::forward<decltype(data)>(data)); }
};