#ifndef CORE_SCENE_H
#define CORE_SCENE_H

#include <list>
#include <vector>
#include <memory>
#include <string>
#include "geometry.h"
#include "interaction.h"
#include "primitive.h"
#include "light.h"
#include "bvh.h"

class Scene {
public:
	Scene();

	bool Intersect(const Ray& ray, float* tHit, SurfaceInteraction* isect) const;
	bool Intersect(const Ray& ray, int except_id) const;

	void AddLight(std::shared_ptr<Light>, const std::string& name);
	void AddInfiniteLight(std::shared_ptr<Light> light, const std::string& name);
	void AddPrimitive(std::shared_ptr<Primitive>, const std::string &name);
	void PrintScene();
	void InitBVH();

	int SetNodesStructure(int structure);

	std::vector<std::shared_ptr<Light>> lights;
	std::vector<std::shared_ptr<Light>> infiniteLights;
	std::vector<std::shared_ptr<Primitive>> primitives;

	std::shared_ptr<BVH> bvh = nullptr;

	int nodes_structure; // 0 brute force 1 bvh
	int current_id;
};


#endif