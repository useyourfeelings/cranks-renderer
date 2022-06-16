#ifndef CORE_APP_H
#define CORE_APP_H

// pbr app main data

#include <vector>
#include "scene.h"
#include "camera.h"
#include "film.h"
#include "scene.h"
#include "sampler.h"
#include "integrator.h"
#include "geometry.h"
#include "../tool/logger.h"

class PbrApp {
public:
	PbrApp(){
		Log("PbrApp()");
		camera = nullptr;
		sampler = nullptr;
		integrator = nullptr;
		//integrator = nullptr;

		scene = std::make_unique<Scene>();

	}


	void SayHi() {
		Log("PbrApp hi");
	}

	void PrintScene();


	//

	void AddSphere(std::string& name, float radius, Point3f position);
	
	void AddPointLight(std::string& name, Point3f position);

	//

	void RenderScene();

	//
	
	void SetFilm(int width, int height);

	void SetCamera(Point3f pos, Point3f look, Vector3f up);
	void SetPerspectiveCamera(Point3f pos, Point3f look, Vector3f up);

	void SetSampler();
	void SetRandomSampler();
	void SetIntegrator();
	void SetWhittedIntegrator();


	std::unique_ptr<Scene> scene;
	std::shared_ptr<Camera> camera;
	std::shared_ptr<Sampler> sampler;
	std::unique_ptr<Integrator> integrator;
	std::vector<std::shared_ptr<Film>> film_list;
};

static PbrApp app;

#endif