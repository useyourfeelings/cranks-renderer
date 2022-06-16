
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

int PBR_API_add_sphere(std::string& name, float radius, float x, float y, float z) {
	app.AddSphere(name, radius, Point3f(x, y, z));
	return 0;
}

int PBR_API_add_point_light(std::string& name, float x, float y, float z) {
	app.AddPointLight(name, Point3f(x, y, z));
	return 0;
}


int PBR_API_render() {
	Log("PBR_API_render");
	app.RenderScene();
	return 0;
}