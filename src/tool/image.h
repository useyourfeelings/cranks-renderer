#ifndef TOOL_IMAGE_H
#define TOOL_IMAGE_H

#include <memory>
#include"../core/geometry.h"
#include"../core/spectrum.h"

unsigned char * ReadHDRRaw(const std::string& name, int* res_x, int* res_y);
std::unique_ptr<RGBSpectrum[]> ReadHDR(const std::string& name, int* res_x, int* res_y);


#endif