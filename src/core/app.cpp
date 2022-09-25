#include <memory>
#include <filesystem>
#include "app.h"
#include "geometry.h"
#include "../light/point.h"
#include "../light/infinite.h"
#include "../shape/sphere.h"
#include "../shape/triangle.h"
#include "../integrator/whitted.h"
#include "../integrator/path.h"
#include "texture.h"
#include "../tool/logger.h"
#include "../tool/image.h"
#include "../tool/model.h"
#include "setting.h"

PbrApp::PbrApp() :
	project_changed(true) {
	std::cout << "PbrApp::PbrApp" << std::endl;
	//Log("PbrApp()"); // can cause exception. mutex not initialized. global variable problem.
	camera = nullptr;
	sampler = nullptr;
	integrator = nullptr;

	material_list = std::make_shared<std::map<int, std::shared_ptr<Material>>>();
	scene = std::make_unique<Scene>(material_list);
	camera = std::shared_ptr<Camera>(new PerspectiveCamera());
	SetSampler();
	SetIntegrator();

	// default material
	json defualt_material = json({
		{"id", 1},
		{"name", "defualt_material"},
		{"type", "matte"},
		{"kd", {0.8, 0.8, 0.8}},
		{"sigma", 0.2},
		});
	auto material = GenMaterial(defualt_material, 1, 1);
	(*material_list)[material->GetID()] = material;
}

int PbrApp::SaveProject(const std::string& path, const std::string& name) {
	auto scene_tree = this->scene->GetSceneTree();

	json project_json;

	project_json["name"] = name;
	project_json["scene"] = scene_tree;

	json m = json::object();
	for (const auto& [key, material] : *material_list) {
		m[std::to_string(key)] = material->GetJson();
	}
	project_json["material"] = m;

	try {
		std::ofstream f(path, std::fstream::trunc);

		if (f.is_open()) {
			f << std::setw(4) << project_json << std::endl;

			std::cout << "Setting.SaveFile() ok" << std::endl;
			return 1;
		}

		f.close();
	}
	catch (const std::exception& e) {
		std::cout << "SaveProject() exception " << e.what();

		return 1;
	}

	return 0;
}

int PbrApp::LoadProject(const std::string& path) {
	std::cout << "LoadProject()" << std::endl;

	std::fstream f;
	f.open(path, std::fstream::in | std::fstream::out);

	if (!f.is_open())
		return 0;

	f.seekp(0);

	json project_json;

	try {
		f >> project_json;
	}
	catch (json::type_error& e) {
		std::cout << e.what() << std::endl << "exception id: " << e.id << std::endl;
	}
	catch (...) {
		std::cout << "LoadProject() exception" << std::endl;
	}

	f.close();

	material_list->clear();

	if (project_json.contains("material")) {
		for (auto& [node_id, node] : project_json["material"].items()) {
			auto material = GenMaterial(node, 1, 1);
			(* material_list)[material->GetID()] = material;
		}
	}
	
	scene->Reload(project_json["scene"]);

	return 0;
}

std::tuple<int, json> PbrApp::AddObjectToScene(const json& obj_info) {
	return this->scene->AddObject(obj_info);
}

int PbrApp::DeleteObjectFromScene(const json& obj_info) {
	return this->scene->DeleteObject(obj_info);
}

int PbrApp::UpdateSceneObject(const json& obj_info) {
	return this->scene->UpdateObject(obj_info);
}

std::string PbrApp::RenameObject(int obj_id, const std::string& new_name) {
	return this->scene->RenameObject(obj_id, new_name);
}

const json& PbrApp::GetSceneTree() {
	return this->scene->GetSceneTree();
}

json PbrApp::GetMaterialTree() {
	json m = json::object();
	for (const auto& [material_id, material] : *material_list) {
		//if(material_id != 1)
		m[std::to_string(material_id)] = material->GetJson();
	}

	return m;
}

json PbrApp::NewMaterial() {
	json defualt_material = json({
		{"name", "new material"},
		{"type", "matte"},
		{"kd", {0.8, 0.8, 0.8}},
		{"sigma", 0.2},
		});
	auto material = GenMaterial(defualt_material, 0, 1);

	(*material_list)[material->GetID()] = material;

	return material->GetJson();
}

int PbrApp::UpdateMaterial(const json& m_info) {
	std::cout << "UpdateMaterial " << m_info << std::endl;
	//auto m = (*material_list)[m_info["id"]];
	//m->Update(m_info);

	if (material_list->contains(m_info["id"])) {
		std::cout << "UpdateMaterial 1" << std::endl;
		//(*material_list)[m_info["id"]].reset(GenMaterial(m_info, 1).get()); // reset. kill all weak_ptr
		(*material_list)[m_info["id"]].reset();
		(*material_list)[m_info["id"]] = GenMaterial(m_info, 1, 0); // reset. kill all weak_ptr
	}

	return 0;
}

int PbrApp::DeleteMaterial(const json& m_info) {
	std::cout << "DeleteMaterial " << m_info << std::endl;

	if (m_info["id"] == 1)
		return 0; // keep default material

	material_list->erase(m_info["id"]);



	return 0;
}

std::string PbrApp::RenameMaterial(int material_id, const std::string& new_name) {
	if (material_list->contains(material_id)) {
		return (*material_list)[material_id]->Rename(new_name);
	}
	else {
		throw("invalid material 1");
	}
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

	scene->InitBVH();

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

void PbrApp::SetSceneOptions(int nodes_structure) {
	scene->SetNodesStructure(nodes_structure);
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