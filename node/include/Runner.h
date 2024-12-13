#pragma once

#include <oneapi/tbb/concurrent_queue.h>

#include <functional>
#include <iostream>
#include <memory>
#include <thread>

#include "Node.h"

/**
 * @class Runner
 * @brief Template class for pipeline nodes that present input data or do stuff outside the pipeline.
 *
 * This class can receive input data from Pusher nodes and other Processor nodes and present the input data i.e. make data available outside the pipeline.
 * This can be done synchronously or asynchronously and requires a prior connection.
 * In asynchronous procedures the input node fills a concurrent queue.
 * This queue is read from by the operating thread created by this class and the input data can be presented.
 * In synchronous procedures the thread of the input node performs the presenting.
 *
 * @attention When an asynchronous procedure is desired, operator()() has to be called first to start the thread.
 * @attention This node has to be synchronously connected by calling synchronously_connect or asynchronously connected by calling asynchronously_connect to other pipeline nodes to receive input data.
 *
 * @tparam Input The input type.
 */
template <typename Input>
class Runner : public Node {
	template <typename T>
	friend class Pusher;
	template <typename T1, typename T2>
	friend class Processor;

	/**
	 * @brief Thread used for running the Runner node asynchronously.
	 */
	std::thread thread;
	/**
	 * @brief Concurrent queue in which other pipeline nodes can input data asynchronously
	 */
	tbb::concurrent_bounded_queue<std::shared_ptr<Input const>> asynchronous_queue;

   public:
	/**
	 * @brief Starts the Runner node's processing loop in a separate thread.
	 */
	[[maybe_unused]] void operator()() {
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

	~Runner() override = default;

   private:
	/**
	 * @brief Does one iteration of this Runner node.
	 */
	void synchronous_call(Input const& input) { run(input); }
	/**
	 * @brief Does one iteration of this Runner node.
	 */
	void synchronous_call_once(Input const& input) { run_once(input); }
	/**
	 * @brief Puts the input data in the concurrent queue to be read from this Runner node's thread asynchronously.
	 */
	void asynchronous_call(std::shared_ptr<Input const> input) {
		if (!asynchronous_queue.try_push(std::forward<decltype(input)>(input))) std::cout << typeid(*this).name() << " skipped!" << std::endl;
	}

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