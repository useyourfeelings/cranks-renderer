#ifndef CORE_API_H
#define CORE_API_H

#include <string>
#include"../tool/json.h"
// api for client

struct CameraSetting {
	float pos[3] = { 0, 0, 0 };
	float look[3] = { 0, 0, 10 };
	float up[3] = { 0, 1, 0 };
	float fov = 90;
	float asp = 1.2f;
	float near_far[2] = { 2, 200 };
	int ray_sample_no = 1;
	int ray_bounce_no = 1;
	int image_scale = 6;
	int resolution[2] = { image_scale * 128, image_scale * 128 };

	int render_threads_count = 3;
};

int PBR_API_hi();
int PBR_API_print_scene();
//int PBR_API_render();
void PBR_API_render(const json&);
int PBR_API_stop_rendering();
int PBR_API_add_sphere(const std::string &name, float radius, float x, float y, float z, const json& material_config);
int PBR_API_add_triangle_mesh(const std::string& name, float x, float y, float z, const std::string& file_name, const nlohmann::json& material_config);
int PBR_API_add_point_light(const std::string& name, float x, float y, float z);
int PBR_API_add_infinite_light(const std::string& name, float x, float y, float z, float r, float g, float b, float strength, float nSample, const std::string& texmap);
int PBR_API_set_perspective_camera(const CameraSetting &s);
int PBR_API_get_camera_setting(CameraSetting &s);
int PBR_API_get_defualt_camera_setting(CameraSetting& s);
int PBR_API_save_setting();
int PBR_API_get_render_progress(int * status, std::vector<int>& now, std::vector<int>& total, int* has_new_photo);
int PBR_API_get_new_image(unsigned char* dst);
int PBR_API_make_test_mipmap(const std::string& name);
int PBR_API_get_mipmap_image(int index, std::vector<unsigned char>& data, int& x, int& y);

#endif