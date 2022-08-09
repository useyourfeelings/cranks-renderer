#ifndef CORE_APP_H
#define CORE_APP_H

// pbr app main data

#include <iostream>
#include <vector>
#include <mutex>
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

class PbrApp {
public:
	PbrApp(){
		std::cout << "PbrApp::PbrApp" << std::endl;
		//Log("PbrApp()"); // can cause exception. mutex not initialized. global variable problem.
		camera = nullptr;
		sampler = nullptr;
		integrator = nullptr;

		scene = std::make_unique<Scene>();
		camera = std::shared_ptr<Camera>(new PerspectiveCamera());
		SetSampler();
		SetIntegrator();
	}

	void SayHi() {
		Log("PbrApp hi");
	}

	void PrintScene();

	//
	std::shared_ptr<Material> GenMaterial(const json& material_config);


	void AddSphere(const std::string& name, float radius, Point3f position, const json& material_config);
	void AddTriangleMesh(const std::string& name, Point3f pos, int tri_count, int vertex_count, int* vertex_index, float* points, const json& material_config);
	
	void AddPointLight(const std::string& name, Point3f position);
	void AddInfiniteLight(const std::string& name, Point3f pos, const Spectrum& power, float strength, int nSamples, const std::string& texmap);
	//

	void RenderScene();
	void GetRenderProgress(int* status, std::vector<int>& now, std::vector<int>& total, int* has_new_photo);
	void StopRendering();
	//
	
	void SetFilm(int width, int height);

	void SetCamera(Point3f pos, Point3f look, Vector3f up);
	void SetPerspectiveCamera(Point3f pos, Point3f look, Vector3f up, 
		float fov, float aspect_ratio, float near, float far, 
		int resX, int resY, int ray_sample_no, int ray_bounce_no, int render_threads_count);

	void SetSampler();
	void SetRandomSampler();
	void SetIntegrator();
	void SetWhittedIntegrator();
	void SetPathIntegrator();
	void SaveSetting();

	//
	void MakeTestMipmap(const std::string& file_name);
	void GetTestMipmapImage(int index, std::vector<unsigned char>& data, int& x, int& y);

	std::unique_ptr<Scene> scene;
	std::shared_ptr<Camera> camera;
	std::shared_ptr<Sampler> sampler;
	std::unique_ptr<Integrator> integrator;
	std::vector<std::shared_ptr<Film>> film_list;

	std::unique_ptr<MIPMap<RGBSpectrum>> test_mipmap = nullptr;

	std::mutex image_mutex;

	int SendNewImage(unsigned char* dst);
};

inline PbrApp app;

#endif