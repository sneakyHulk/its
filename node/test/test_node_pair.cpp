#include <chrono>
#include <iostream>
#include <thread>

#include "Pusher.h"
#include "RunnerSynchronousPair.h"

using namespace std::chrono_literals;

class Push1 : public Pusher<int> {
	int i = 0;

   public:
	Push1() { std::cout << "push1()" << std::endl; }
	int push() final {
		std::cout << ++i << " push1.push()" << std::endl;
		return i;
	}
	int push_once() final {
		std::cout << ++i << " push1.push_once()" << std::endl;
		return i;
	}
};

class Push2 : public Pusher<float> {
	float f = 0;

   public:
	Push2() { std::cout << "push2()" << std::endl; }
	float push() final {
		std::cout << ++f << " push2.push()" << std::endl;
		return f;
	}
	float push_once() final {
		std::cout << ++f << " push2.push_once()" << std::endl;
		return f;
	}
};

class Run1 : public RunnerSynchronousPair<int, float> {
   public:
	Run1() { std::cout << "run1()" << std::endl; }
	void run(int const& data1, float const& data2) final { std::cout << "run1.run(" << data1 << ", " << data2 << ")" << std::endl; }
};

int main() {
	{
		Push1 push1;
		Push2 push2;
		Run1 run1;

		push1.synchronously_connect(run1);
		push2.synchronously_connect(run1);

		auto t1 = push1();
		auto t2 = push2();

		std::this_thread::sleep_for(100ms);
	}

	std::this_thread::sleep_for(1000ms);
}