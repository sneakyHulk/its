#pragma once

#include <gst/app/gstappsink.h>
#include <gst/gst.h>

#include "node.h"
#include "ImageData.h"
#include "StreamingNodeBase.h"

class ReceivingImageNode : public Pusher<ImageData>, StreamingNodeBase {

};