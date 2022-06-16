#include <memory>
#include <filesystem>
#include "app.h"
#include "geometry.h"
#include "../light/point.h"
#include "../shape/sphere.h"
#include "../integrator/whitted.h"
#include "../tool/logger.h"

void PbrApp::AddSphere(std::string &name, float radius, Point3f pos) {
	Log("AddSphere %s", name.c_str());

	auto t = Translate(Vector3f(pos.x, pos.y, pos.z));

	//Float radius = params.FindOneFloat("radius", 1.f);
	float zmin = -radius;
	float zmax = radius;
	float phimax = 360.f;

	std::shared_ptr<Shape> shape = std::make_shared<Sphere>(t, Inverse(t), radius, zmin, zmax, phimax);

	auto material = nullptr; //std::make_shared<MatteMaterial>();

	scene->AddPrimitive(std::make_shared<GeometricPrimitive>(shape, material), name);
}

void PbrApp::AddPointLight(std::string& name, Point3f pos) {
	auto t = Translate(Vector3f(pos.x, pos.y, pos.z));

	this->scene->AddLight(std::make_shared<PointLight>(t, Spectrum(1.0)), name);
}

void PbrApp::PrintScene() {
	scene->PrintScene();
}

void PbrApp::RenderScene() {
	//std::string str = GetCurrentWorkingDir();

	Log("RenderScene");
	Log("%s", std::filesystem::current_path().c_str());

	SetFilm(100, 100);
	SetCamera(Point3f(10, 0, 0), Point3f(10, 0, 10), Vector3f(0, 1, 0));
	SetSampler();
	SetIntegrator();

	integrator->Render(*scene);
}

void PbrApp::SetFilm(int width, int height) {
	Log("SetFilm");
	film_list.push_back(std::make_shared<Film>(Point2i(width, height)));
}

void PbrApp::SetCamera(Point3f pos, Point3f look, Vector3f up) {
	if (camera != nullptr)
		return;

	SetPerspectiveCamera(pos, look, up);
}

void PbrApp::SetPerspectiveCamera(Point3f pos, Point3f look, Vector3f up) {
	Log("SetPerspectiveCamera");
	Point2f screenMin, screenMax;
	float fov = 90;
	float aspect_ratio = 1;// 1.6;
	float near = 0.01, far = 100;

	if (film_list.empty() || film_list.back()->status != 0)
		SetFilm(100, 100);

	auto film = film_list.back();

	float aspectRatio = film->fullResolution.x / film->fullResolution.y;

	if (aspectRatio > 1.f) {
		screenMin.x = -aspectRatio;
		screenMax.x = aspectRatio;
		screenMin.y = -1.f;
		screenMax.y = 1.f;
	}
	else {
		screenMin.x = -1.f;
		screenMax.x = 1.f;
		screenMin.y = -1.f / aspectRatio;
		screenMax.y = 1.f / aspectRatio;
	}

	//this->camera = std::make_unique<PerspectiveCamera>(fov, film, near, far);

	//Transform t = Translate(Vector3f(pos.x, pos.y, pos.z));

	Transform t = Inverse(LookAt(pos, look, up));

	this->camera = std::shared_ptr<Camera>(new PerspectiveCamera(t, BBox2f(screenMin, screenMax), fov, aspect_ratio, film, near, far));

}

void PbrApp::SetSampler() {
	if (this->sampler != nullptr)
		return;

	SetRandomSampler();
}

void PbrApp::SetRandomSampler() {
	Log("SetRandomSampler");

	this->sampler = std::shared_ptr<Sampler>(new RandomSampler(1));
}

void PbrApp::SetIntegrator() {
	if (this->integrator != nullptr)
		return;

	SetWhittedIntegrator();
}

void PbrApp::SetWhittedIntegrator() {
	Log("SetWhittedIntegrator");

	int maxDepth = 5;
	BBox2i bounds(Point2i(0, 0), this->camera->film->fullResolution);

	this->integrator = std::unique_ptr<Integrator>(new WhittedIntegrator(maxDepth, camera, sampler, bounds));
}