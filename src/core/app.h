#ifndef CORE_APP_H
#define CORE_APP_H

// pbr app main data

#include <iostream>
#include <vector>
#include <mutex>
#include <map>
#include <tuple>
#include "scene.h"
#include "camera.h"
#include "film.h"
#include "scene.h"
#include "sampler.h"
#include "integrator.h"
#include "geometry.h"
#include "../tool/logger.h"
#include "setting.h"
#include "mipmap.h"
#include "material.h"

class PbrApp {
public:
	PbrApp();

	void SayHi() {
		Log("PbrApp hi");
	}

	void PrintScene();

	int SaveProject(const std::string& path, const std::string& name);
	int LoadProject(const std::string& path);
	std::tuple<int, json> AddObjectToScene(const json& obj_info);
	int DeleteObjectFromScene(const json& obj_info);
	int UpdateSceneObject(const json& obj_info);
	const json& GetSceneTree();
	json GetMaterialTree();
	json NewMaterial();
	int UpdateMaterial(const json& m_info);
	int DeleteMaterial(const json& m_info);
	std::string RenameMaterial(int material_id, const std::string& new_name);
	std::string RenameObject(int obj_id, const std::string& new_name);

	json GetMediumTree();
	json NewMedium();
	int UpdateMedium(const json& m_info);
	int DeleteMedium(const json& m_info);
	std::string RenameMedium(int medium_id, const std::string& new_name);

	//

	void RenderScene();
	void GetRenderProgress(int* status, std::vector<int>& now, std::vector<int>& total, std::vector<float>& per, int* has_new_photo, json& render_status_info);
	void StopRendering();
	//
	
	void SetFilm(int width, int height);

	void SetCamera(Point3f pos, Point3f look, Vector3f up);
	void SetPerspectiveCamera(Point3f pos, Point3f look, Vector3f up, 
		float fov, float aspect_ratio, float near, float far, 
		int resX, int resY, int medium_id);

	void SetSceneOptions(int nodes_structure);

	void SetSampler();
	void SetRandomSampler();
	void InitIntegrator();
	int SetIntegrator(const json& info);

	int SetSystemConfig(const json& config);

	//void SetWhittedIntegrator();
	//void SetPathIntegrator();
	//void SetPMIntegrator();
	void SelectIntegrator(int method);

	json GetSceneConfig();
	json GetIntegratorConfig(int id);
	json GetSystemConfig();

	void SaveSetting();

	//
	void MakeTestMipmap(const std::string& file_name);
	void GetTestMipmapImage(int index, std::vector<unsigned char>& data, int& x, int& y);

	std::shared_ptr<Scene> scene;
	std::shared_ptr<Camera> camera;
	std::shared_ptr<Sampler> sampler;
	//std::vector<std::shared_ptr<Film>> film_list;
	std::unique_ptr<MIPMap<RGBSpectrum>> test_mipmap = nullptr;
	std::mutex image_mutex;
	std::shared_ptr<std::map<int, std::shared_ptr<Material>>> material_list;

	std::shared_ptr<std::map<int, std::shared_ptr<Medium>>> medium_list;

	std::vector<std::unique_ptr<Integrator>> integrators; // 0-whitted 1-path 2-pm 3-ppm 4-vpath
	//std::unique_ptr<Integrator> whittedIntegrator;
	//std::unique_ptr<Integrator> pathIntegrator;
	//std::unique_ptr<Integrator> pmIntegrator;

	int SendNewImage(unsigned char* dst);

private:
	bool project_changed;
	int render_method = 2; // 0-whitted 1-path 2-pm

	int render_threads_no = 12;
};

inline PbrApp app;

#endif