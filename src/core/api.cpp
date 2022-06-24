
#include "app.h"
#include "error.h"
#include "../tool/logger.h"
#include "geometry.h"
#include "geometry.h"
#include "setting.h"

//extern PbrApp app;

int PBR_API_hi() {
	app.SayHi();
	return 0;
}

int PBR_API_print_scene() {
	app.PrintScene();

	return 0;
}

int PBR_API_add_sphere(const std::string& name, float radius, float x, float y, float z) {
	app.AddSphere(name, radius, Point3f(x, y, z));
	return 0;
}

int PBR_API_add_point_light(const std::string& name, float x, float y, float z) {
	app.AddPointLight(name, Point3f(x, y, z));
	return 0;
}

int PBR_API_set_perspective_camera(float* pos, float* look, float* up,
	float fov, float asp, float near, float far,
	int resX, int resY, int ray_sample_no) {
	Log("PBR_API_set_perspective_camera %f %f %f", pos[0], pos[1], pos[2]);
	app.SetPerspectiveCamera(Point3f(pos[0], pos[1], pos[2]),
		Point3f(look[0], look[1], look[2]),
		Vector3f(up[0], up[1], up[2]),
		fov, asp, near, far, resX, resY, ray_sample_no);

	//auto a = { pos[0], pos[1], pos[2] };
	setting.Set("camera_pos", std::initializer_list({ pos[0], pos[1], pos[2] }));
	setting.Set("camera_look", std::initializer_list({ look[0], look[1], look[2] }));
	setting.Set("camera_up", std::initializer_list({up[0], up[1], up[2]}));
	setting.Set("camera_fov", fov);
	setting.Set("camera_asp", asp);
	setting.Set("camera_near", near);
	setting.Set("camera_far", far);
	setting.Set("camera_resX", resX);
	setting.Set("camera_resY", resY);
	setting.Set("ray_sample_no", ray_sample_no);
	Log(setting.Dump().c_str());
	return 0;
}

int PBR_API_save_setting() {
	app.SaveSetting();
	return 0;
}

int PBR_API_get_camera_setting(float* pos, float* look, float* up, 
	float& fov, float& asp, float& near, float& far, 
	int& resX, int& resY, int& ray_sample_no) {

	pos[0] = setting.Get("camera_pos")[0];
	pos[1] = setting.Get("camera_pos")[1];
	pos[2] = setting.Get("camera_pos")[2];

	look[0] = setting.Get("camera_look")[0];
	look[1] = setting.Get("camera_look")[1];
	look[2] = setting.Get("camera_look")[2];

	up[0] = setting.Get("camera_up")[0];
	up[1] = setting.Get("camera_up")[1];
	up[2] = setting.Get("camera_up")[2];

	fov = setting.Get("camera_fov");
	asp = setting.Get("camera_asp");
	near = setting.Get("camera_near");
	far = setting.Get("camera_far");

	resX = setting.Get("camera_resX");
	resY = setting.Get("camera_resY");

	ray_sample_no = setting.Get("ray_sample_no");

	return 0;
}

int PBR_API_get_defualt_camera_setting(float* pos, float* look, float* up, 
	float& fov, float& asp, float& near, float& far, 
	int& resX, int& resY, int& ray_sample_no) {
	Log("PBR_API_get_defualt_camera_setting");

	setting.LoadDefaultSetting();
	PBR_API_get_camera_setting(pos, look, up,
		fov, asp, near, far,
		resX, resY, ray_sample_no);

	Log(setting.Dump().c_str());
	
	return 0;
}


int PBR_API_render() {
	Log("PBR_API_render");
	app.RenderScene();
	Log("PBR_API_render over");
	return 0;
}

int PBR_API_get_render_progress(int *now, int *total) {
	app.GetRenderProgress(now, total);
	return 0;
}