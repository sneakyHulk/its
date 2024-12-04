#pragma once

#include <gst/app/gstappsink.h>
#include <gst/gst.h>

#include "node.h"
#include "ImageData.h"

class ReceivingImageNode : public Pusher<ImageData> {

};