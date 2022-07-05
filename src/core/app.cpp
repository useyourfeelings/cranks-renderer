#include <memory>
#include <filesystem>
#include "app.h"
#include "geometry.h"
#include "../light/point.h"
#include "../shape/sphere.h"
#include "../integrator/whitted.h"
#include "../tool/logger.h"
#include "setting.h"

void PbrApp::AddSphere(const std::string &name, float radius, Point3f pos) {
	Log("AddSphere %s", name.c_str());

	auto t = Translate(Vector3f(pos.x, pos.y, pos.z));

	//Float radius = params.FindOneFloat("radius", 1.f);
	float zmin = -radius;
	float zmax = radius;
	float phimax = 360.f;

	std::shared_ptr<Shape> shape = std::make_shared<Sphere>(t, Inverse(t), radius, zmin, zmax, phimax);

	auto material = std::make_shared<MatteMaterial>();

	scene->AddPrimitive(std::make_shared<GeometricPrimitive>(shape, material), name);
}

void PbrApp::AddPointLight(const std::string& name, Point3f pos) {
	auto t = Translate(Vector3f(pos.x, pos.y, pos.z));

	this->scene->AddLight(std::make_shared<PointLight>(t, Spectrum(1.0)), name);
}

void PbrApp::PrintScene() {
	scene->PrintScene();
}

void PbrApp::GetRenderProgress(int* status, int* now, int* total, int * has_new_photo) {
	if (this->integrator != nullptr) {
		*status = this->integrator->render_status;
		*now = this->integrator->render_progress_now;
		*total = this->integrator->render_progress_total;
		*has_new_photo = this->integrator->has_new_photo;
	}
	else {
		*status = 0;
		*now = 0;
		*total = 0;
		*has_new_photo = 0;
	}
	
}

void PbrApp::StopRendering() {
	integrator->render_status = 0;
}

void PbrApp::RenderScene() {
	//std::string str = GetCurrentWorkingDir();

	Log("RenderScene");
	Log("%s", std::filesystem::current_path().c_str());

	//SetCamera(Point3f(0, 0, 0), Point3f(0, 0, 10), Vector3f(0, 1, 0));

	if (camera == nullptr) {
		setting.LoadDefaultSetting();
		/*SetPerspectiveCamera(Point3f(setting.Get("camera_pos")[0], default_setting.camera_pos[1], default_setting.camera_pos[2]),
			Point3f(default_setting.camera_look[0], default_setting.camera_look[1], default_setting.camera_look[2]),
			Vector3f(default_setting.camera_up[0], default_setting.camera_up[1], default_setting.camera_up[2]),
			default_setting.camera_fov, default_setting.camera_aspect_ratio, default_setting.camera_near, default_setting.camera_far,
			default_setting.camera_resX, default_setting.camera_resY);*/

		SetPerspectiveCamera(Point3f(setting.Get("camera_pos")[0], setting.Get("camera_pos")[1], setting.Get("camera_pos")[2]),
			Point3f(setting.Get("camera_look")[0], setting.Get("camera_look")[1], setting.Get("camera_look")[2]),
			Vector3f(setting.Get("camera_up")[0], setting.Get("camera_up")[1], setting.Get("camera_up")[2]),
			setting.Get("camera_fov"), setting.Get("camera_asp"), setting.Get("camera_near"), 
			setting.Get("camera_far"), setting.Get("camera_resX"), setting.Get("camera_resY"), setting.Get("ray_sample_no"));
	}

	SetFilm(camera->resolutionX, camera->resolutionY);

	SetIntegrator();

	integrator->Render(*scene);
}

void PbrApp::SetFilm(int width, int height) {
	Log("SetFilm");
	film_list.push_back(std::make_shared<Film>(Point2i(width, height)));
	camera->SetFilm(film_list.back());
}

void PbrApp::SetCamera(Point3f pos, Point3f look, Vector3f up) {
	if (camera != nullptr)
		return;

	//SetPerspectiveCamera(pos, look, up);
}

void PbrApp::SetPerspectiveCamera(Point3f pos, Point3f look, Vector3f up, 
	float fov, float aspect_ratio, float near, float far, 
	int resX, int resY, int ray_sample_no) {
	Log("SetPerspectiveCamera");

	sampler->SetSamplesPerPixel(ray_sample_no);

	if (camera != nullptr) {
		camera->SetPerspectiveData(pos, look, up, fov, aspect_ratio, near, far, resX, resY);
		return;
	}


	/*if (film_list.empty() || film_list.back()->status != 0)
		SetFilm(100, 100);

	auto film = film_list.back();*/


	// LookAtÊÇworld to camera
	//Transform camera2World = Inverse(LookAt(pos, look, up));

	//this->camera = std::shared_ptr<Camera>(new PerspectiveCamera(camera2World, BBox2f(screenMin, screenMax), fov, aspect_ratio, film, near, far));

	this->camera = std::shared_ptr<Camera>(new PerspectiveCamera(pos, look, up, fov, aspect_ratio, near, far, resX, resY));

	//this->sampler->SetSamplesPerPixel(ray_sample_no);

	//SetFilm(100, 100);
}

void PbrApp::SetSampler() {
	if (this->sampler != nullptr)
		return;

	SetRandomSampler();
}

void PbrApp::SetRandomSampler() {
	//Log("SetRandomSampler");

	this->sampler = std::shared_ptr<Sampler>(new RandomSampler(setting.Get("ray_sample_no"))); // 8 better than 4. 4 better than 1.
}

void PbrApp::SetIntegrator() {
	if (this->integrator != nullptr)
		return;

	SetWhittedIntegrator();
}

void PbrApp::SetWhittedIntegrator() {
	Log("SetWhittedIntegrator");

	int maxDepth = 1;
	BBox2i bounds(Point2i(0, 0), Point2i(camera->resolutionX, camera->resolutionY));

	this->integrator = std::unique_ptr<Integrator>(new WhittedIntegrator(maxDepth, camera, sampler, bounds));
}

void PbrApp::SaveSetting() {
	setting.SaveFile();
}

int PbrApp::SendNewImage(char* dst) {
	std::lock_guard<std::mutex> lock(image_mutex);

	if (!this->integrator->has_new_photo) {
		return 1;
	}

	this->integrator->has_new_photo = 0;

	this->camera->film->WriteVector(dst);

	return 0;
}