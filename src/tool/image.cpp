#include "image.h"
#include<iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "../../third_party/stb/stb_image.h"

unsigned char* ReadHDRRaw(const std::string& name, int* res_x, int* res_y) {
    std::cout << "ReadHDR..." << std::endl;

    auto is_hdr = stbi_is_hdr(name.c_str());

    std::cout << "is_hdr " << is_hdr << std::endl;

    int comp;

    auto res = stbi_load(name.c_str(), res_x, res_y, &comp, 4); // rgb

    std::cout << "stbi_load " << *res_x << " " << *res_y << " " << comp << std::endl;

    return res;
}

std::unique_ptr<RGBSpectrum[]> ReadHDR(const std::string& name, int* res_x, int* res_y) {
    std::cout << "ReadHDR..." << std::endl;

    auto is_hdr = stbi_is_hdr(name.c_str());

    std::cout << "is_hdr " << is_hdr << std::endl;

    int comp;

    auto res = stbi_load(name.c_str(), res_x, res_y, &comp, 3); // rgb

    std::cout << "stbi_load " << *res_x <<" "<< *res_y<<" "<< comp << std::endl;

    return nullptr;
}