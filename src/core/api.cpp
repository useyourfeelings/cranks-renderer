
#include "app.h"
#include "api.h"
#include "error.h"
#include "../tool/logger.h"
#include "geometry.h"
#include "spectrum.h"
#include "setting.h"
#include "../base/events.h"

int PBR_API_hi() {
	app.SayHi();
	return 0;
}

int PBR_API_print_scene() {
	app.PrintScene();

	return 0;
}

int PBR_API_save_project(const std::string& path, const std::string& name) {
	app.SaveProject(path, name);
	return 0;
}

int PBR_API_load_project(const std::string& path) {
	app.LoadProject(path);
	return 0;
}

std::tuple<int, json> PBR_API_add_object_to_scene(const json& obj_info) {
	return app.AddObjectToScene(obj_info);
}

int PBR_API_delete_object_from_scene(const json& obj_info) {
	app.DeleteObjectFromScene(obj_info);
	return 0;
}

int PBR_API_update_scene_object(const json& obj_info) {
	app.UpdateSceneObject(obj_info);
	return 0;
}

const json& PBR_API_get_scene_tree() {
	return app.GetSceneTree();
}

json PBR_API_get_material_tree() {
	return app.GetMaterialTree();
}

json PBR_API_new_material() {
	return app.NewMaterial();
}

int PBR_API_update_material(const json& m_info) {
	app.UpdateMaterial(m_info);
	return 0;
}

int PBR_API_delete_material(const json& obj_info) {
	app.DeleteMaterial(obj_info);
	return 0;
}

std::string PBR_API_rename_material(int material_id, const std::string& new_name) {
	return app.RenameMaterial(material_id, new_name);
}

std::string PBR_API_rename_object(int obj_id, const std::string& new_name) {
	return app.RenameObject(obj_id, new_name);
}

json PBR_API_get_medium_tree() {
	return app.GetMediumTree();
}

json PBR_API_new_medium() {
	return app.NewMedium();
}

int PBR_API_update_medium(const json& m_info) {
	app.UpdateMedium(m_info);
	return 0;
}

int PBR_API_delete_medium(const json& medium_info) {
	app.DeleteMedium(medium_info);
	return 0;
}

std::string PBR_API_rename_medium(int medium_id, const std::string& new_name) {
	return app.RenameMedium(medium_id, new_name);
}

int PBR_API_set_perspective_camera(const CameraSetting& s) {
	Log("PBR_API_set_perspective_camera %f %f %f", s.pos[0], s.pos[1], s.pos[2]);
	app.SetPerspectiveCamera(Point3f(s.pos[0], s.pos[1], s.pos[2]),
		Point3f(s.look[0], s.look[1], s.look[2]),
		Vector3f(s.up[0], s.up[1], s.up[2]),
		s.fov, s.asp, s.near_far[0], s.near_far[1], s.resolution[0], s.resolution[1], s.medium_id);

	//auto a = { pos[0], pos[1], pos[2] };
	setting.Set("camera_pos", std::initializer_list({ s.pos[0], s.pos[1], s.pos[2] }));
	setting.Set("camera_look", std::initializer_list({ s.look[0], s.look[1], s.look[2] }));
	setting.Set("camera_up", std::initializer_list({ s.up[0], s.up[1], s.up[2] }));
	setting.Set("camera_fov", s.fov);
	setting.Set("camera_asp", s.asp);
	setting.Set("camera_near", s.near_far[0]);
	setting.Set("camera_far", s.near_far[1]);
	setting.Set("camera_resX", s.resolution[0]);
	setting.Set("camera_resY", s.resolution[1]);
	//setting.Set("ray_sample_no", s.ray_sample_no);
	//setting.Set("ray_bounce_no", s.ray_bounce_no); 
	//setting.Set("render_threads_no", s.render_threads_no);

	Log(setting.Dump().c_str());
	return 0;
}

int PBR_API_SET_SCENE_OPTIONS(const SceneOptions& s) {
	app.SetSceneOptions(s.nodes_structure);

	return 0;
}

int PbrApiSetSystemConfig(const SystemConfig& config) {
	json system_config;
	system_config["render_threads_no"] = config.render_threads_no;
	
	app.SetSystemConfig(system_config);
	return 0;
}

int PbrApiGetSystemConfig(SystemConfig& config) {
	json system_config = app.GetSystemConfig();
	config.render_threads_no = system_config["render_threads_no"];

	return 0;
}

int PbrApiSelectIntegrator(int method) {
	app.SelectIntegrator(method);
	return 0;
}

int PbrApiSetIntegrator(const json& info) {
	app.SetIntegrator(info);
	return 0;
}

int PBR_API_save_setting() {
	app.SaveSetting();
	return 0;
}

int PBR_API_get_camera_setting(CameraSetting& s) {
	s.pos[0] = setting.Get("camera_pos")[0];
	s.pos[1] = setting.Get("camera_pos")[1];
	s.pos[2] = setting.Get("camera_pos")[2];

	s.look[0] = setting.Get("camera_look")[0];
	s.look[1] = setting.Get("camera_look")[1];
	s.look[2] = setting.Get("camera_look")[2];

	s.up[0] = setting.Get("camera_up")[0];
	s.up[1] = setting.Get("camera_up")[1];
	s.up[2] = setting.Get("camera_up")[2];

	s.fov = setting.Get("camera_fov");
	s.asp = setting.Get("camera_asp");
	s.near_far[0] = setting.Get("camera_near");
	s.near_far[1] = setting.Get("camera_far");

	s.resolution[0] = setting.Get("camera_resX");
	s.resolution[1] = setting.Get("camera_resY");

	s.image_scale = setting.Get("image_scale");

	return 0;
}

int PBR_API_get_defualt_camera_setting(CameraSetting& s) {
		Log("PBR_API_get_defualt_camera_setting");

		setting.LoadDefaultSetting();
		PBR_API_get_camera_setting(s);

		Log(setting.Dump().c_str());

		return 0;
}

void PBR_API_render(const MultiTaskCtx& args) {
	Log("PBR_API_render");
	app.RenderScene();
	Log("PBR_API_render over");
}

int PBR_API_get_render_progress(int* status, std::vector<int>& now, std::vector<int>& total, std::vector<float>& per, int * has_new_photo, json & render_status_info) {
	app.GetRenderProgress(status, now, total, per, has_new_photo, render_status_info);
	return 0;
}

int PBR_API_stop_rendering() {
	app.StopRendering();
	return 0;
}

int PBR_API_get_new_image(unsigned char* dst) {
	app.SendNewImage(dst);
	return 0;
}

int PBR_API_make_test_mipmap(const std::string& name) {
	app.MakeTestMipmap(name);
	return 0;
}

int PBR_API_get_mipmap_image(int index, std::vector<unsigned char>& data, int &x, int &y) {
	app.GetTestMipmapImage(index, data, x, y);
	return 0;
}

int PbrApiGetSceneConfig(SceneOptions& config) {
	json scene_config = app.GetSceneConfig();
	config.nodes_structure = scene_config["nodes_structure"];
	config.render_method = scene_config["render_method"];

	//config.render_threads_no = 12;

	return 0;
}

int PbrApiGetIntegratorsConfig(WhittedIntConfig& whitted_config, 
	PathIntConfig& path_onfig, VpathIntConfig& vpath_config, PMIntConfig& pm_config, PPMIntConfig& ppm_config) {

	// todo: clean index

	json config = app.GetIntegratorConfig(0);
	std::cout << config << std::endl;

	whitted_config.ray_bounce_no = config["ray_bounce_no"];
	whitted_config.ray_sample_no = config["ray_sample_no"];
	//whitted_config.render_threads_no = config["render_threads_no"];

	config = app.GetIntegratorConfig(1);
	std::cout << config << std::endl;

	path_onfig.ray_bounce_no = config["ray_bounce_no"];
	path_onfig.ray_sample_no = config["ray_sample_no"];
	//path_onfig.render_threads_no = config["render_threads_no"];

	config = app.GetIntegratorConfig(4);
	std::cout << config << std::endl;

	vpath_config.ray_bounce_no = config["ray_bounce_no"];
	vpath_config.ray_sample_no = config["ray_sample_no"];
	//vpath_config.render_threads_no = config["render_threads_no"];

	config = app.GetIntegratorConfig(2);
	std::cout << config << std::endl;

	pm_config.ray_sample_no = config["ray_sample_no"];
	pm_config.ray_bounce_no = config["ray_bounce_no"];
	//pm_config.render_threads_no = config["render_threads_no"];
	pm_config.emit_photons = config["emit_photons"];
	pm_config.gather_photons = config["gather_photons"];
	pm_config.gather_photons_r = config["gather_photons_r"];
	pm_config.gather_method = config["gather_method"];
	pm_config.energy_scale = config["energy_scale"];
	pm_config.reemit_photons = config["reemit_photons"];
	pm_config.render_direct = config["render_direct"];
	pm_config.render_specular = config["render_specular"];
	pm_config.render_caustic = config["render_caustic"];
	pm_config.render_diffuse = config["render_diffuse"];
	pm_config.render_global = config["render_global"];
	pm_config.filter = config["filter"];
	pm_config.specular_method = config["specular_method"];
	pm_config.specular_rt_samples = config["specular_rt_samples"];


	config = app.GetIntegratorConfig(3);
	std::cout << config << std::endl;

	//ppm_config.render_threads_no = config["render_threads_no"];
	ppm_config.ray_bounce_no = config["ray_bounce_no"];
	ppm_config.energy_scale = config["energy_scale"];
	ppm_config.render_direct = config["render_direct"];
	ppm_config.render_caustic = config["render_caustic"];
	ppm_config.render_diffuse = config["render_diffuse"];
	ppm_config.render_global = config["render_global"];
	ppm_config.filter = config["filter"];
	ppm_config.max_iterations = config["max_iterations"];
	ppm_config.inital_radius = config["inital_radius"];
	ppm_config.alpha = config["alpha"];
	//ppm_config.render_threads_no = config["render_threads_no"];

	return 0;
}


//int PbrApiGetWhittedIntConfig(WhittedIntConfig& config) {
//
//}
//
//int PbrApiGetPathIntConfig(PathIntConfig& config) {
//
//}
//
//int PbrApiGetVpathIntConfig(VpathIntConfig& config) {
//
//}
//
//int PbrApiGetPMIntConfig(PMIntConfig& config) {
//
//}
//
//int PbrApiGetPPMIntConfig(PPMIntConfig& config) {
//
//}
