#pragma once

#include "CompactObject.h"

#include <yas/serialize.hpp>
#include <yas/std_types.hpp>

YAS_DEFINE_INTRUSIVE_SERIALIZE("CompactObject", CompactObject, id, object_class, position, velocity)
YAS_DEFINE_INTRUSIVE_SERIALIZE("CompactObjects", CompactObjects, timestamp, objects)