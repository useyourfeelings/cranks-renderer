#include "error.h"
//#include <cassert>

void Error(const char* format, ...) {
	throw(format);
   //assert("format");
}