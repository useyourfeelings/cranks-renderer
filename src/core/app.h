#ifndef CORE_APP_H
#define CORE_APP_H

// pbr app main data

#include <iostream>
#include <vector>
#include "scene.h"
#include "camera.h"
#include "film.h"
#include "scene.h"
#include "sampler.h"
#include "integrator.h"
#include "geometry.h"
#include "../tool/logger.h"
#include "setting.h"

class PbrApp {
public:
	PbrApp(){
		std::cout << "PbrApp::PbrApp" << std::endl;
		//Log("PbrApp()"); // can cause exception. mutex not initialized. global variable problem.
		camera = nullptr;
		sampler = nullptr;
		integrator = nullptr;

		scene = std::make_unique<Scene>();

	}

	void SayHi() {
		Log("PbrApp hi");
	}

	void PrintScene();


	//

	void AddSphere(const std::string& name, float radius, Point3f position);
	
	void AddPointLight(const std::string& name, Point3f position);

	//

	void RenderScene();
	void GetRenderProgress(int* now, int* total);

	//
	
	void SetFilm(int width, int height);

	void SetCamera(Point3f pos, Point3f look, Vector3f up);
	void SetPerspectiveCamera(Point3f pos, Point3f look, Vector3f up, 
		float fov, float aspect_ratio, float near, float far, 
		int resX, int resY, int ray_sample_no);

	void SetSampler();
	void SetRandomSampler();
	void SetIntegrator();
	void SetWhittedIntegrator();
	void SaveSetting();

	std::unique_ptr<Scene> scene;
	std::shared_ptr<Camera> camera;
	std::shared_ptr<Sampler> sampler;
	std::unique_ptr<Integrator> integrator;
	std::vector<std::shared_ptr<Film>> film_list;

	
};

inline PbrApp app;

#endif