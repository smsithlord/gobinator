// The unit tests' one assertion: it counts a failure and carries on.
#pragma once

#include <cstdio>

extern int failures;

#define CHECK(cond) \
	do { if (!(cond)) { std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); ++failures; } } while (0)
