#ifndef CORE_SCENE_H
#define CORE_SCENE_H

#include <list>
#include <memory>
#include <string>
#include "geometry.h"
#include "interaction.h"
#include "primitive.h"
#include "light.h"


class Scene {
public:
	Scene();

	bool Intersect(const Ray& ray, float* tHit, SurfaceInteraction* isect) const;
	bool Intersect(const Ray& ray, int except_id) const;

	void AddLight(std::shared_ptr<Light>, std::string& name);
	void AddPrimitive(std::shared_ptr<Primitive>, std::string &name);
	void PrintScene();

	std::list<std::shared_ptr<Light>> lights;
	std::list<std::shared_ptr<Primitive>> primitives;

	int current_id;
};


#endif