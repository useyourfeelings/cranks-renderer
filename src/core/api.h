#ifndef CORE_API_H
#define CORE_API_H

#include <string>

// api for client

int PBR_API_hi();
int PBR_API_print_scene();
int PBR_API_render();
int PBR_API_add_sphere(std::string &name, float radius, float x, float y, float z);
int PBR_API_add_point_light(std::string& name, float x, float y, float z);

#endif