#include <array>
#include <string>

#include "Detection2D.h"
#include "ImageData.h"

template <int height, int width>
Detections2D run_yolo(ImageData const& image);