#include "scene.h"
#include "../tool/logger.h"

Scene::Scene():
	nodes_structure(1),
	current_id(0) {
}

int Scene::SetNodesStructure(int structure) {
	if (structure == 0 || structure == 1) {
		std::cout<<"SetNodesStructure "<< structure << std::endl;
		nodes_structure = structure;
		return 0;
	}

	return -1;
}

void Scene::AddLight(std::shared_ptr<Light> light, const std::string& name) {
	lights.push_back(light);
}

void Scene::AddInfiniteLight(std::shared_ptr<Light> light, const std::string& name) {
	lights.push_back(light);
	infiniteLights.push_back(light);
}

void Scene::AddPrimitive(std::shared_ptr<Primitive> p, const std::string& name) {
	p->SetId(++ current_id);
	p->SetName(name);
	Log("AddPrimitive %d %s", current_id, p->name.c_str());
	primitives.push_back(p);
}

void Scene::InitBVH() {
	if (bvh == nullptr) {
		bvh = std::make_shared<BVH>(primitives);
	}
	
}

void Scene::PrintScene() {
	Log("PrintScene");
	Log("current_id %d", current_id);
	for (auto p : primitives) {
		Log("id = %d, name = %s", p->id, p->name.c_str());
	}
}

bool Scene::Intersect(const Ray& ray, float* tHit, SurfaceInteraction* isect) const {
	if(nodes_structure == 1)
		return bvh->Intersect(ray, tHit, isect);

	// 遍历所有。找最小的t。

	SurfaceInteraction isect_result;
	SurfaceInteraction isect_temp;

	float t_temp;
	float t_result = MaxFloat;

	for (auto p : primitives) {
		if (p->Intersect(ray, &t_temp, &isect_temp)) {
			if (t_temp < t_result) {
				t_result = t_temp;
				isect_result = isect_temp;
			}
		}
	}

	if (t_result == MaxFloat)
		return false;

	*tHit = t_result;
	*isect = isect_result;

	return true;
}

bool Scene::Intersect(const Ray& ray, int except_id) const {
	for (auto p : primitives) {
		if (p->id == except_id)
			continue;

		if (p->Intersect(ray))
			return false;
	}

	return true;
}
