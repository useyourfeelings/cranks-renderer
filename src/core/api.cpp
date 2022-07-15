
#include "app.h"
#include "api.h"
#include "error.h"
#include "../tool/logger.h"
#include "geometry.h"
#include "geometry.h"
#include "setting.h"

int PBR_API_hi() {
	app.SayHi();
	return 0;
}

int PBR_API_print_scene() {
	app.PrintScene();

	return 0;
}

int PBR_API_add_sphere(const std::string& name, float radius, float x, float y, float z, const nlohmann::json& material_config) {
	app.AddSphere(name, radius, Point3f(x, y, z), material_config);
	return 0;
}

int PBR_API_add_point_light(const std::string& name, float x, float y, float z) {
	app.AddPointLight(name, Point3f(x, y, z));
	return 0;
}

int PBR_API_set_perspective_camera(const CameraSetting& s) {
	Log("PBR_API_set_perspective_camera %f %f %f", s.pos[0], s.pos[1], s.pos[2]);
	app.SetPerspectiveCamera(Point3f(s.pos[0], s.pos[1], s.pos[2]),
		Point3f(s.look[0], s.look[1], s.look[2]),
		Vector3f(s.up[0], s.up[1], s.up[2]),
		s.fov, s.asp, s.near_far[0], s.near_far[1], s.resolution[0], s.resolution[1], s.ray_sample_no, s.ray_bounce_no);

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
	setting.Set("ray_sample_no", s.ray_sample_no);
	setting.Set("ray_bounce_no", s.ray_bounce_no);
	Log(setting.Dump().c_str());
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

	s.ray_sample_no = setting.Get("ray_sample_no");
	s.ray_bounce_no = setting.Get("ray_bounce_no");

	return 0;
}

int PBR_API_get_defualt_camera_setting(CameraSetting& s) {
		Log("PBR_API_get_defualt_camera_setting");

		setting.LoadDefaultSetting();
		PBR_API_get_camera_setting(s);

		Log(setting.Dump().c_str());

		return 0;
}

int PBR_API_render() {
	Log("PBR_API_render");
	app.RenderScene();
	Log("PBR_API_render over");
	return 0;
}

int PBR_API_get_render_progress(int* status, int *now, int *total, int * has_new_photo) {
	app.GetRenderProgress(status, now, total, has_new_photo);
	return 0;
}

int PBR_API_stop_rendering() {
	app.StopRendering();
	return 0;
}

int PBR_API_get_new_image(char* dst) {
	app.SendNewImage(dst);
	return 0;
}
