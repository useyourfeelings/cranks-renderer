
#include "app.h"
#include "error.h"
#include "../tool/logger.h"
#include "geometry.h"

extern PbrApp app;

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

int PBR_API_set_perspective_camera(float* pos, float* look, float* up, float fov, float aspect_ratio, float near, float far, int resX, int resY) {
	Log("PBR_API_set_perspective_camera %f %f %f", pos[0], pos[1], pos[2]);
	app.SetPerspectiveCamera(Point3f(pos[0], pos[1], pos[2]), 
		Point3f(look[0], look[1], look[2]), 
		Vector3f(up[0], up[1], up[2]),
		fov, aspect_ratio, near, far, resX, resY);
	return 0;
}

int PBR_API_get_defualt_camera_setting(float* pos, float* look, float* up, float& fov, float& aspect_ratio, float& near, float& far, int& resX, int& resY) {
	Log("PBR_API_get_defualt_camera_setting");
	pos[0] = app.default_setting.camera_pos[0];
	pos[1] = app.default_setting.camera_pos[1];
	pos[2] = app.default_setting.camera_pos[2];

	look[0] = app.default_setting.camera_look[0];
	look[1] = app.default_setting.camera_look[1];
	look[2] = app.default_setting.camera_look[2];

	up[0] = app.default_setting.camera_up[0];
	up[1] = app.default_setting.camera_up[1];
	up[2] = app.default_setting.camera_up[2];

	fov = app.default_setting.camera_fov;
	aspect_ratio = app.default_setting.camera_aspect_ratio;
	near = app.default_setting.camera_near;
	far = app.default_setting.camera_far;

	resX = app.default_setting.camera_resX;
	resY = app.default_setting.camera_resY;
	
	return 0;
}


int PBR_API_render() {
	Log("PBR_API_render");
	app.RenderScene();
	Log("PBR_API_render over");
	return 0;
}