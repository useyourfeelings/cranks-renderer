#ifndef CORE_API_H
#define CORE_API_H

#include <string>

// api for client

int PBR_API_hi();
int PBR_API_print_scene();
int PBR_API_render();
int PBR_API_add_sphere(const std::string &name, float radius, float x, float y, float z);
int PBR_API_add_point_light(const std::string& name, float x, float y, float z);
int PBR_API_set_perspective_camera(float* pos, float* look, float* up, float fov, float aspect_ratio, float near, float far, int resX, int resY);
int PBR_API_get_defualt_camera_setting(float* pos, float* look, float* up, float& fov, float& aspect_ratio, float& near, float& far, int& resX, int& resY);

#endif