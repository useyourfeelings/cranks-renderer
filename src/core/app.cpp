#include <memory>
#include <filesystem>
#include "app.h"
#include "geometry.h"
#include "../light/point.h"
#include "../light/infinite.h"
#include "../shape/sphere.h"
#include "../integrator/whitted.h"
#include "../integrator/path.h"
#include "texture.h"
#include "../tool/logger.h"
#include "../tool/image.h"
#include "setting.h"

std::shared_ptr<Material> PbrApp::GenMaterial(const json& material_config) {
	std::cout << "PbrApp::GenMaterial()" << material_config.dump() << std::endl;

	std::shared_ptr<Material> material = nullptr;

	auto material_name = material_config["name"];

	if (material_name == "matte") {
		auto kd = material_config["kd"];
		std::shared_ptr<Texture<Spectrum>> kd_tex = std::make_shared<ConstantTexture<Spectrum>>(Spectrum(kd[0], kd[1], kd[2]));

		std::shared_ptr<Texture<float>> sigma_tex = std::make_shared<ConstantTexture<float>>(material_config["sigma"]);

		material = std::make_shared<MatteMaterial>(kd_tex, sigma_tex, nullptr);
	} else if (material_name == "glass") {
		auto kr = material_config["kr"];
		auto kt = material_config["kt"];
		auto remaproughness = material_config["remaproughness"];
		std::shared_ptr<Texture<Spectrum>> kr_tex = std::make_shared<ConstantTexture<Spectrum>>(Spectrum(kr[0], kr[1], kr[2]));
		std::shared_ptr<Texture<Spectrum>> kt_tex = std::make_shared<ConstantTexture<Spectrum>>(Spectrum(kt[0], kt[1], kt[2]));

		std::shared_ptr<Texture<float>> eta = std::make_shared<ConstantTexture<float>>(material_config["eta"]);
		std::shared_ptr<Texture<float>> uroughness = std::make_shared<ConstantTexture<float>>(material_config["uroughness"]);
		std::shared_ptr<Texture<float>> vroughness = std::make_shared<ConstantTexture<float>>(material_config["vroughness"]);
		std::shared_ptr<Texture<float>> bumpmap = std::make_shared<ConstantTexture<float>>(material_config["bumpmap"]);

		material = std::make_shared<GlassMaterial>(kr_tex, kt_tex, uroughness, vroughness, eta, bumpmap, remaproughness);
	}
	else if (material_name == "mirror") {
		auto kr = material_config["kr"];
		std::shared_ptr<Texture<Spectrum>> kr_tex = std::make_shared<ConstantTexture<Spectrum>>(Spectrum(kr[0], kr[1], kr[2]));

		std::shared_ptr<Texture<float>> bumpmap = std::make_shared<ConstantTexture<float>>(material_config["bumpmap"]);

		material = std::make_shared<MirrorMaterial>(kr_tex, bumpmap);
	}


	return material;
}

void PbrApp::AddSphere(const std::string &name, float radius, Point3f pos, const json& material_config) {
	Log("AddSphere %s", name.c_str());

	auto t = Translate(Vector3f(pos.x, pos.y, pos.z));

	//Float radius = params.FindOneFloat("radius", 1.f);
	float zmin = -radius;
	float zmax = radius;
	float phimax = 360.f;

	std::shared_ptr<Shape> shape = std::make_shared<Sphere>(t, Inverse(t), radius, zmin, zmax, phimax);

	//Log("material ")

	//std::shared_ptr<Material> material = 

	scene->AddPrimitive(std::make_shared<GeometricPrimitive>(shape, GenMaterial(material_config)), name);
}

void PbrApp::AddPointLight(const std::string& name, Point3f pos) {
	auto t = Translate(Vector3f(pos.x, pos.y, pos.z));

	this->scene->AddLight(std::make_shared<PointLight>(t, Spectrum(1, 1, 1)), name);
}

void PbrApp::AddInfiniteLight(const std::string& name, Point3f pos, const Spectrum& power, float strength,
	int nSamples, const std::string& texmap) {
	auto t = Translate(Vector3f(pos.x, pos.y, pos.z));

	this->scene->AddInfiniteLight(std::make_shared<InfiniteAreaLight>(t, power, strength, nSamples, texmap), name);
}

void PbrApp::PrintScene() {
	scene->PrintScene();
}

void PbrApp::GetRenderProgress(int* status, std::vector<int>& now, std::vector<int>& total, int * has_new_photo) {
	if (this->integrator != nullptr) {
		*status = this->integrator->render_status;
		//*now = this->integrator->GetRenderProgress();
		//*total = this->integrator->render_progress_total;
		*has_new_photo = this->integrator->has_new_photo;

		if (now.size() != this->integrator->render_threads_count)
			now.resize(this->integrator->render_threads_count);

		if (total.size() != this->integrator->render_threads_count)
			total.resize(this->integrator->render_threads_count);

		for (int i = 0; i < now.size(); ++i) {
			now[i] = this->integrator->render_progress_now[i];
		}

		for (int i = 0; i < total.size(); ++i) {
			total[i] = this->integrator->render_progress_total[i];
		}
	}
	else {
		*status = 0;
		//*now = 0;
		//*total = 0;
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
			setting.Get("camera_far"), setting.Get("camera_resX"), setting.Get("camera_resY"), 
			setting.Get("ray_sample_no"), setting.Get("ray_bounce_no"), setting.Get("render_threads_count"));
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
	int resX, int resY, int ray_sample_no, int ray_bounce_no, int render_threads_count) {
	Log("SetPerspectiveCamera");

	sampler->SetSamplesPerPixel(ray_sample_no);
	integrator->SetRayBounceNo(ray_bounce_no);
	integrator->SetRenderThreadsCount(render_threads_count);

	if (camera != nullptr) {
		camera->SetPerspectiveData(pos, look, up, fov, aspect_ratio, near, far, resX, resY);
		return;
	}


	/*if (film_list.empty() || film_list.back()->status != 0)
		SetFilm(100, 100);

	auto film = film_list.back();*/


	//this->camera = std::shared_ptr<Camera>(new PerspectiveCamera(pos, look, up, fov, aspect_ratio, near, far, resX, resY));

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

	//SetWhittedIntegrator();
	SetPathIntegrator();
}

void PbrApp::SetWhittedIntegrator() {
	//Log("SetWhittedIntegrator");

	int maxDepth = 1;
	BBox2i bounds(Point2i(0, 0), Point2i(camera->resolutionX, camera->resolutionY));

	this->integrator = std::unique_ptr<Integrator>(new WhittedIntegrator(maxDepth, camera, sampler, bounds));
}

void PbrApp::SetPathIntegrator() {
	//Log("SetPathIntegrator");

	int maxDepth = 3;
	BBox2i bounds(Point2i(0, 0), Point2i(camera->resolutionX, camera->resolutionY));
	float rrThreshold = 0;

	this->integrator = std::unique_ptr<Integrator>(new PathIntegrator(maxDepth, camera, sampler, bounds, rrThreshold, "uniform"));
}

void PbrApp::SaveSetting() {
	setting.SaveFile();
}

int PbrApp::SendNewImage(unsigned char* dst) {
	std::lock_guard<std::mutex> lock(image_mutex);

	if (!this->integrator->has_new_photo) {
		return 1;
	}

	this->integrator->has_new_photo = 0;

	this->camera->film->WriteVector(dst);

	return 0;
}

void PbrApp::MakeTestMipmap(const std::string& file_name) {
	int res_x, res_y;
	auto image_data = ReadHDRRaw(file_name, &res_x, &res_y);

	auto spectrum_data = new RGBSpectrum[res_x * res_y];
	for (int i = 0; i < res_x * res_y; ++i) {
		spectrum_data[i] = RGBSpectrum(image_data[i * 4] / 255.f, image_data[i * 4 + 1] / 255.f, image_data[i * 4 + 2] / 255.f);
	}

	this->test_mipmap = std::make_unique<MIPMap<RGBSpectrum>>(Point2i(res_x, res_y), spectrum_data);


	delete[]spectrum_data;
}

void PbrApp::GetTestMipmapImage(int index, std::vector<unsigned char>& data, int& x, int& y) {
	this->test_mipmap->getMipmapImage(index, data, x, y);
}