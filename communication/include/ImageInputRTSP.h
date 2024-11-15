#pragma once
#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include <gst/gst.h>

#include <bitset>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <string>
#include <thread>

#include "common_exception.h"
#include "common_output.h"
#include "node.h"

class ImageInputRTSP : public