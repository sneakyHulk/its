#include <gpiod.h>
#include <pylon/BaslerUniversalInstantCamera.h>
#include <pylon/GrabResultPtr.h>
#include <pylon/PylonIncludes.h>
#include <stdio.h>
#include <unistd.h>

#include <csignal>
#include <functional>

#include "common_exception.h"
#include "common_output.h"

int main(int argc, char **argv) {
	struct gpiod_chip *chip = gpiod_chip_open_by_name("gpiochip0");
	if (!chip) {
		throw common::Exception("Open chip failed.");
	}

	struct gpiod_line *line = gpiod_chip_get_line(chip, 24);  // GPIO Pin #24
	if (!line) {
		gpiod_chip_close(chip);

		throw common::Exception("Get line failed.");
	}

	int ret = gpiod_line_request_output(line, "camera", 0);
	if (ret < 0) {
		gpiod_line_release(line);
		gpiod_chip_close(chip);

		throw common::Exception("Request line as output failed.");
	}

	common::print("Initialize Pylon...");
	Pylon::PylonInitialize();
	common::println("done!");

	Pylon::DeviceInfoList_t device_list;
	Pylon::CTlFactory::GetInstance().EnumerateDevices(device_list);
	if (device_list.empty()) {
		gpiod_line_release(line);
		gpiod_chip_close(chip);

		common::print("Terminate Pylon...");
		Pylon::PylonTerminate();
		common::println("done!");

		throw common::Exception("No Basler camera devices found!");
	}

	try {
		for (auto &device : device_list) {
		}
	} catch (Pylon::GenericException const &e) {
		gpiod_line_release(line);
		gpiod_chip_close(chip);

		common::print("Terminate Pylon...");
		Pylon::PylonTerminate();
		common::println("done!");

		std::rethrow_exception(std::current_exception());
	}

	/* Blink 20 times */
	// val = 0;
	// for (i = 20; i > 0; i--) {
	//	ret = gpiod_line_set_value(line, val);
	//	if (ret < 0) {
	//		perror("Set line output failed\n");
	//		goto release_line;
	//	}
	//	printf("Output %u on line #%u\n", val, line_num);
	//	sleep(1);
	//	val = !val;
	// }

	return 0;
}