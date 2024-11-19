#include <gpiod.h>
#include <pylon/BaslerUniversalInstantCamera.h>
#include <pylon/GrabResultPtr.h>
#include <pylon/PylonIncludes.h>
#include <stdio.h>
#include <unistd.h>

#include <chrono>
#include <csignal>
#include <functional>
#include <opencv2/opencv.hpp>
#include <thread>

#include "common_exception.h"
#include "common_output.h"
std::function<void()> clean_up;

int main(int argc, char **argv) {
	struct gpiod_chip *chip = gpiod_chip_open_by_name("gpiochip0");
	if (!chip) {
		throw common::Exception("Open chip failed.");
	}

	clean_up = [&chip]() { gpiod_chip_close(chip); };
	std::signal(SIGINT, [](int sig) { clean_up(); });
	std::signal(SIGTERM, [](int sig) { clean_up(); });

	struct gpiod_line *line = gpiod_chip_get_line(chip, 24);  // GPIO Pin #24
	if (!line) {
		gpiod_chip_close(chip);

		throw common::Exception("Get line failed.");
	}

	clean_up = [&chip, &line]() {
		gpiod_line_release(line);
		gpiod_chip_close(chip);
	};
	std::signal(SIGINT, [](int sig) { clean_up(); });
	std::signal(SIGTERM, [](int sig) { clean_up(); });

	int ret = gpiod_line_request_output(line, "camera", 0);
	if (ret < 0) {
		gpiod_line_release(line);
		gpiod_chip_close(chip);

		throw common::Exception("Request line as output failed.");
	}

	common::print("Initialize Pylon...");
	Pylon::PylonInitialize();
	common::println("done!");

	clean_up = [&chip, &line]() {
		gpiod_line_release(line);
		gpiod_chip_close(chip);

		common::print("Terminate Pylon...");
		Pylon::PylonTerminate();
		common::println("done!");
	};
	std::signal(SIGINT, [](int sig) { clean_up(); });
	std::signal(SIGTERM, [](int sig) { clean_up(); });

	Pylon::DeviceInfoList_t device_list;
	Pylon::CTlFactory::GetInstance().EnumerateDevices(device_list);
	if (device_list.empty()) {
		clean_up();

		throw common::Exception("No Basler camera devices found!");
	}

	try {
		Pylon::CBaslerUniversalInstantCamera camera;
		for (auto &device : device_list) {

			if (device.GetMacAddress() != "0030534C1B61") continue;

			camera.Attach(Pylon::CTlFactory::GetInstance().CreateDevice(device));
			camera.Open();

			camera.TriggerSelector.SetValue(Basler_UniversalCameraParams::TriggerSelector_FrameStart);
			// Enable triggered image acquisition for the Frame Start trigger
			camera.TriggerMode.SetValue(Basler_UniversalCameraParams::TriggerMode_On);
			// Set the trigger source to Line 1
			camera.TriggerSource.SetValue(Basler_UniversalCameraParams::TriggerSource_Line1);
			// Set the trigger activation mode to rising edge
			camera.TriggerActivation.SetValue(Basler_UniversalCameraParams::TriggerActivation_RisingEdge);

			while (true) {
				gpiod_line_set_value(line, 1);
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				gpiod_line_set_value(line, 0);

				Pylon::CGrabResultPtr ptrGrabResult;
				camera.RetrieveResult(1000, ptrGrabResult);

				if (!ptrGrabResult->GrabSucceeded()) {
					common::println(ptrGrabResult->GetErrorDescription());

					continue;
				}

				std::vector<std::uint8_t> image_data(static_cast<const std::uint8_t *>(ptrGrabResult->GetBuffer()), static_cast<const std::uint8_t *>(ptrGrabResult->GetBuffer()) + ptrGrabResult->GetBufferSize());

				cv::Mat const image(1200, 1920, CV_8UC1, image_data.data());

				cv::imshow("image", image);
				cv::waitKey(0);
			}
		}
	} catch (Pylon::GenericException const &e) {
		clean_up();

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