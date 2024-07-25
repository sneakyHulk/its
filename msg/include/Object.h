#pragma once

#include <cstdint>

class Object {
   public:
	std::uint64_t id;
};

class IntObject : public Object {
   public:
	int i;
};

class FloatObject : public Object {
   public:
	float f;
};