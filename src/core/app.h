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

// raw data
class PbrInfo {
public:
	/*DefaultPbrInfo():camera_pos(0, 0, 0),
		camera_look(0, 0, 10),
		camera_up(0, 1, 0),
		camera_fov(90),
		camera_aspect_ratio(1.2),
		camera_near(1),
		camera_far(200)
	{
	};

	Point3f camera_pos, camera_look;
	Vector3f camera_up;
	float camera_fov, camera_aspect_ratio, camera_near, camera_far;*/

	PbrInfo() :
		camera_fov(90),
		camera_aspect_ratio(1),
		camera_near(1),
		camera_far(200),
		camera_resX(200),
		camera_resY(200)
	{
		camera_pos[0] = 0;
		camera_pos[1] = -20; // 0
		camera_pos[2] = 0; // -20

		camera_look[0] = 0; // 0
		camera_look[1] = 0; // 0
		camera_look[2] = 0; // 10

		camera_up[0] = 0; // 0;
		camera_up[1] = 0;//  1;
		camera_up[2] = 1;//  0;
	};

	float camera_pos[3], camera_look[3], camera_up[3];
	float camera_aspect_ratio, camera_near, camera_far;
	float camera_fov;
	int camera_resX, camera_resY;
};

class PbrApp {
public:
	PbrApp(){
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

	void AddSphere(std::string& name, float radius, Point3f position);
	
	void AddPointLight(std::string& name, Point3f position);

	//

	void RenderScene();

	//
	
	void SetFilm(int width, int height);

	void SetCamera(Point3f pos, Point3f look, Vector3f up);
	void SetPerspectiveCamera(Point3f pos, Point3f look, Vector3f up, float fov, float aspect_ratio, float near, float far, int resX, int resY);

	void SetSampler();
	void SetRandomSampler();
	void SetIntegrator();
	void SetWhittedIntegrator();


	std::unique_ptr<Scene> scene;
	std::shared_ptr<Camera> camera;
	std::shared_ptr<Sampler> sampler;
	std::unique_ptr<Integrator> integrator;
	std::vector<std::shared_ptr<Film>> film_list;

	PbrInfo default_setting;
	PbrInfo setting;
};

static PbrApp app;

#endif