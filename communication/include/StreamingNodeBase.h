#pragma once

#include <thread>

class StreamingNodeBase {
	inline static std::thread loop_thread;

   protected:
	StreamingNodeBase();
};
