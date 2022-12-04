#ifndef CORE_API_H
#define CORE_API_H

#include <string>
#include <tuple>
#include"../tool/json.h"
#include"../base/events.h"
// api for client

struct CameraSetting {
	float pos[3] = { 0, 0, 0 };
	float look[3] = { 0, 0, 10 };
	float up[3] = { 0, 1, 0 };
	float fov = 90;
	float asp = 1.2f;
	float near_far[2] = { 2, 200 };
	int image_scale = 2;
	int resolution[2] = { image_scale * 128, image_scale * 128 };
};

struct SceneOptions {
	int nodes_structure = 1; // bvh
	int render_method = 2; // 0-whitted 1-path tracing 2-photon mapping 3-ppm

	int ray_sample_no = 1;
	int ray_bounce_no = 12;
	int render_threads_no = 12;

	// pm
	int emitPhotons = 2000;
	int gatherPhotons = 20;
	float gatherPhotonsR = 0.f;
	int gatherMethod = 0;
	float energyScale = 15000;
	bool reemitPhotons = 1;

	bool renderDirect = 1;
	bool renderSpecular = 1;
	bool renderCaustic = 1;
	bool renderDiffuse = 1;
	bool renderGlobal = 0;

	int filter = 1;
	int specularMethod = 1;
	int specularRTSamples = 3;

	// ppm
	int maxIterations = 100;
	float initalRadius = 20;
	float alpha = 0.5;
};

int PBR_API_hi();
int PBR_API_print_scene();
void PBR_API_render(const MultiTaskCtx&);
int PBR_API_stop_rendering();
int PBR_API_save_project(const std::string& path, const std::string& name);
int PBR_API_load_project(const std::string& path);
std::tuple<int, json> PBR_API_add_object_to_scene(const json& obj_info);
int PBR_API_delete_object_from_scene(const json& obj_info);
int PBR_API_update_scene_object(const json& obj_info);
const json& PBR_API_get_scene_tree();
json PBR_API_get_material_tree();
json PBR_API_new_material();
int PBR_API_update_material(const json& m_info);
int PBR_API_delete_material(const json& obj_info);
std::string PBR_API_rename_material(int material_id, const std::string& new_name);
std::string PBR_API_rename_object(int obj_id, const std::string& new_name);
int PBR_API_set_perspective_camera(const CameraSetting &s);
int PBR_API_get_camera_setting(CameraSetting &s);
int PBR_API_get_defualt_camera_setting(CameraSetting& s);
int PBR_API_save_setting();
int PBR_API_get_render_progress(int * status, std::vector<int>& now, std::vector<int>& total, std::vector<float>& per, int* has_new_photo, json& render_status_info);
int PBR_API_get_new_image(unsigned char* dst);
int PBR_API_make_test_mipmap(const std::string& name);
int PBR_API_get_mipmap_image(int index, std::vector<unsigned char>& data, int& x, int& y);
int PBR_API_SET_SCENE_OPTIONS(const SceneOptions& s);
int PbrApiSelectIntegrator(int method);
int PbrApiSetIntegrator(const json& info);

#endif