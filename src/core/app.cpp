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
#include "../integrator/vpath.h"
#include "../integrator/pm.h"
#include "../integrator/ppm.h"
#include "texture.h"
#include "../tool/logger.h"
#include "../tool/image.h"
#include "../tool/model.h"
#include "setting.h"
#include "medium.h"

PbrApp::PbrApp() :
	project_changed(true) {
	std::cout << "PbrApp::PbrApp" << std::endl;
	//Log("PbrApp()"); // can cause exception. mutex not initialized. global variable problem.
	camera = nullptr;
	sampler = nullptr;
	//integrator = nullptr;

	material_list = std::make_shared<std::map<int, std::shared_ptr<Material>>>();
	medium_list = std::make_shared<std::map<int, std::shared_ptr<Medium>>>();

	scene = std::make_unique<Scene>(material_list, medium_list);
	camera = std::shared_ptr<Camera>(new PerspectiveCamera());
	SetSampler();

	//SetWhittedIntegrator();
	//SetPathIntegrator();
	//SetPMIntegrator();
	InitIntegrator();
	SelectIntegrator(render_method);

	// default material
	json defualt_material = json({
		//{"id", 1},
		{"name", "defualt_material"},
		{"type", "matte"},
		{"kd", {0.8, 0.8, 0.8}},
		{"sigma", 0.2},
		});
	//auto material = GenMaterial(defualt_material);
	//(*material_list)[material->GetID()] = material;
}

int PbrApp::SaveProject(const std::string& path, const std::string& name) {
	auto scene_tree = this->scene->GetSceneTree();

	json project_json;

	project_json["name"] = name;
	project_json["scene"] = scene_tree;

	// save material
	json m = json::object();
	for (const auto& [key, material] : *material_list) {
		m[std::to_string(key)] = material->GetJson();
	}
	project_json["material"] = m;

	// save medium
	m = json::object();
	for (const auto& [key, medium] : *medium_list) {
		m[std::to_string(key)] = medium->GetJson();
	}
	project_json["medium"] = m;

	// save object_info
	project_json["object_info"] = json();
	project_json["object_info"]["latest_obj_id"] = Object::GetLatestObjectID();

	// save integrators
	json integrators_json;

	for (int i = 0; i < integrators.size(); ++ i) {
		integrators_json[std::format("{}", i)] = integrators[i]->GetConfig();
	}

	project_json["integrators"] = integrators_json;

	// system config
	json system_config;
	system_config["render_threads_no"] = render_threads_no;

	project_json["system"] = system_config;

	// save project_json
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
	medium_list->clear();

	// set latest_obj_id
	Object::LoadLatestObjectID(project_json["object_info"]["latest_obj_id"]);

	// load material
	if (project_json.contains("material")) {
		for (auto& [node_id, node] : project_json["material"].items()) {
			auto material = GenMaterial(node);
			(* material_list)[material->GetID()] = material;
		}
	}

	// load material
	if (project_json.contains("medium")) {
		for (auto& [node_id, node] : project_json["medium"].items()) {
			auto medium = MakeHomogeneousMedium(node);
			(*medium_list)[medium->GetID()] = medium;
		}
	}
	
	// load scene
	scene->Reload(project_json["scene"]);

	// load integrators
	if (project_json.contains("integrators")) {
		for (auto& [node_id, node] : project_json["integrators"].items()) {
			integrators[std::stoi(node_id)]->SetOptions(node);
		}
	}

	// load system_config
	if (project_json.contains("system")) {
		json system_config = project_json["system"];

		if (system_config.contains("render_threads_no")) {
			render_threads_no = system_config["render_threads_no"];
		}
	}

	for (int i = 0; i < integrators.size(); ++i) {
		integrators[i]->SetRenderThreadsCount(render_threads_no);
	}

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
	auto material = GenMaterial(defualt_material);

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
		(*material_list)[m_info["id"]] = GenMaterial(m_info); // reset. kill all weak_ptr
	}

	return 0;
}

int PbrApp::DeleteMaterial(const json& m_info) {
	std::cout << "DeleteMaterial " << m_info << std::endl;

	//if (m_info["id"] == 1)
	//	return 0; // keep default material

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

// medium

json PbrApp::GetMediumTree() {
	json m = json::object();
	for (const auto& [medium_id, medium] : *medium_list) {
		m[std::to_string(medium_id)] = medium->GetJson();
	}

	return m;
}

json PbrApp::NewMedium() {
	json defualt_medium = json({
		{"name", "new medium"},
		{"type", "homo"},
		{"sigma_a", {0.8, 0.8, 0.8}},
		{"sigma_s", {0.8, 0.8, 0.8}},
		{"g", 0.3},
		});

	auto medium = MakeHomogeneousMedium(defualt_medium);

	(*medium_list)[medium->GetID()] = medium;

	return medium->GetJson();
}

int PbrApp::UpdateMedium(const json& m_info) {
	std::cout << "UpdateMedium " << m_info << std::endl;
	//auto m = (*material_list)[m_info["id"]];
	//m->Update(m_info);

	if (medium_list->contains(m_info["id"])) {
		std::cout << "UpdateMedium 1" << std::endl;
		//(*material_list)[m_info["id"]].reset(GenMaterial(m_info, 1).get()); // reset. kill all weak_ptr
		(*medium_list)[m_info["id"]].reset();
		(*medium_list)[m_info["id"]] = MakeHomogeneousMedium(m_info); // reset. kill all weak_ptr
	}

	return 0;
}

int PbrApp::DeleteMedium(const json& m_info) {
	std::cout << "DeleteMedium " << m_info << std::endl;

	//if (m_info["id"] == 1)
	//	return 0; // keep default material

	medium_list->erase(m_info["id"]);

	return 0;
}

std::string PbrApp::RenameMedium(int medium_id, const std::string& new_name) {
	if (medium_list->contains(medium_id)) {
		return (*medium_list)[medium_id]->Rename(new_name);
	}
	else {
		throw("invalid medium 1");
	}
}

void PbrApp::PrintScene() {
	scene->PrintScene();
}

void PbrApp::GetRenderProgress(int* status, std::vector<int>& now, std::vector<int>& total, std::vector<float>& per, int * has_new_photo, json& render_status_info) {
	if (this->integrators[render_method] != nullptr) {
		*status = this->integrators[render_method]->render_status;
		//*now = this->integrator->GetRenderProgress();
		//*total = this->integrator->render_progress_total;
		*has_new_photo = this->integrators[render_method]->has_new_photo;

		if (now.size() != this->integrators[render_method]->render_threads_no)
			now.resize(this->integrators[render_method]->render_threads_no);

		if (total.size() != this->integrators[render_method]->render_threads_no)
			total.resize(this->integrators[render_method]->render_threads_no);

		if (per.size() != this->integrators[render_method]->render_threads_no)
			per.resize(this->integrators[render_method]->render_threads_no);

		for (int i = 0; i < now.size(); ++i) {
			now[i] = this->integrators[render_method]->render_progress_now[i];
			total[i] = this->integrators[render_method]->render_progress_total[i];
			per[i] = float(now[i]) / total[i];
		}

		//render_status_info = this->integrators[render_method]->GetRenderStatus();
		render_status_info.merge_patch(this->integrators[render_method]->GetRenderStatus());
		
	}
	else {
		*status = 0;
		//*now = 0;
		//*total = 0;
		*has_new_photo = 0;
	}
	
}

void PbrApp::StopRendering() {
	integrators[render_method]->render_status = 0;
}

void PbrApp::RenderScene() {
	std::cout << std::format("PbrApp::RenderScene()\n");
	//std::string str = GetCurrentWorkingDir();

	Log("RenderScene");
	Log("%s", std::filesystem::current_path().c_str());

	//SetCamera(Point3f(0, 0, 0), Point3f(0, 0, 10), Vector3f(0, 1, 0));

	if (camera == nullptr) {
		setting.LoadDefaultSetting();

		SetPerspectiveCamera(Point3f(setting.Get("camera_pos")[0], setting.Get("camera_pos")[1], setting.Get("camera_pos")[2]),
			Point3f(setting.Get("camera_look")[0], setting.Get("camera_look")[1], setting.Get("camera_look")[2]),
			Vector3f(setting.Get("camera_up")[0], setting.Get("camera_up")[1], setting.Get("camera_up")[2]),
			setting.Get("camera_fov"), setting.Get("camera_asp"), setting.Get("camera_near"), 
			setting.Get("camera_far"), setting.Get("camera_resX"), setting.Get("camera_resY"), 0);
	}

	//SetFilm(camera->resolutionX, camera->resolutionY);
	camera->SetFilm();

	scene->InitBVH();

	integrators[render_method]->Render(*scene);
}

void PbrApp::SetFilm(int width, int height) {
	Log("SetFilm");
	//film_list.push_back(std::make_shared<Film>(Point2i(width, height)));
	//camera->SetFilm(film_list.back());
}

void PbrApp::SetCamera(Point3f pos, Point3f look, Vector3f up) {
	if (camera != nullptr)
		return;

	//SetPerspectiveCamera(pos, look, up);
}

void PbrApp::SetPerspectiveCamera(Point3f pos, Point3f look, Vector3f up, 
	float fov, float aspect_ratio, float near, float far, 
	int resX, int resY, int medium_id) {
	Log("SetPerspectiveCamera");

	/*sampler->SetSamplesPerPixel(ray_sample_no);
	integrators[render_method]->SetRayBounceNo(ray_bounce_no);
	integrators[render_method]->SetRenderThreadsCount(render_threads_no);*/

	if (camera != nullptr) {
		camera->SetPerspectiveData(pos, look, up, fov, aspect_ratio, near, far, resX, resY);

		if (medium_list->contains(medium_id)) {
			camera->SetMedium(medium_list->at(medium_id));
		}

		return;
	}

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

void PbrApp::InitIntegrator() {

	// whitted
	int maxDepth = 1;
	BBox2i bounds(Point2i(0, 0), Point2i(camera->resolutionX, camera->resolutionY));

	integrators.push_back(std::make_unique<WhittedIntegrator>(maxDepth, camera, sampler, bounds));

	// path
	maxDepth = 20;
	float rrThreshold = 0;

	integrators.push_back(std::make_unique<PathIntegrator>(maxDepth, camera, sampler, bounds, rrThreshold, "uniform"));

	// pm
	integrators.push_back(std::make_unique<PMIntegrator>(camera, sampler));

	// ppm
	integrators.push_back(std::make_unique<PPMIntegrator>(camera, sampler));

	// vpath
	integrators.push_back(std::make_unique<VolumePathIntegrator>(maxDepth, camera, sampler, bounds, rrThreshold, "uniform"));

}

void PbrApp::SelectIntegrator(int method) {
	render_method = method;
}

int PbrApp::SetIntegrator(const json& info) {
	std::cout << std::format("PbrApp::SetIntegrator()\n");
	if (info.contains("whitted")) {
		integrators[0]->SetOptions(info["whitted"]);
	} else if (info.contains("path")) {
		integrators[1]->SetOptions(info["path"]);
	} else if (info.contains("vpath")) {
		integrators[4]->SetOptions(info["vpath"]);
	} else if (info.contains("pm")) {
		integrators[2]->SetOptions(info["pm"]);
	} else if (info.contains("ppm")) {
		integrators[3]->SetOptions(info["ppm"]);
	}

	return 0;
}

int PbrApp::SetSystemConfig(const json& config) {
	if (config.contains("render_threads_no")) {
		render_threads_no = config["render_threads_no"];

		for (int i = 0; i < integrators.size(); ++i) {
			integrators[i]->SetRenderThreadsCount(int(config["render_threads_no"]));
		}
	}

	return 0;
}

json PbrApp::GetSceneConfig() {
	return scene->GetConfig();
}

json PbrApp::GetSystemConfig() {
	json config;

	config["render_threads_no"] = render_threads_no;

	return config;
}

json PbrApp::GetIntegratorConfig(int id) {
	if(0 <= id && id < integrators.size())
		return integrators[id]->GetConfig();

	return json({});
}

void PbrApp::SaveSetting() {
	setting.SaveFile();
}

int PbrApp::SendNewImage(unsigned char* dst) {
	std::lock_guard<std::mutex> lock(image_mutex);

	if (!this->integrators[render_method]->has_new_photo) {
		return 1;
	}

	this->integrators[render_method]->has_new_photo = 0;

	// todo: lock film

	//this->camera->film->WriteVector(dst);
	this->camera->films.back()->WriteVector(dst);

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