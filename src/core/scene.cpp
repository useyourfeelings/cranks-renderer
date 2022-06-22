#include "scene.h"
#include "../tool/logger.h"

Scene::Scene() {
	current_id = 0;
}

void Scene::AddLight(std::shared_ptr<Light> light, const std::string& name) {
	lights.push_back(light);
}

void Scene::AddPrimitive(std::shared_ptr<Primitive> p, const std::string& name) {
	p->SetId(++ current_id);
	p->SetName(name);
	Log("AddPrimitive %d %s", current_id, p->name.c_str());
	primitives.push_back(p);
}

void Scene::PrintScene() {
	Log("PrintScene");
	Log("current_id %d", current_id);
	for (auto p : primitives) {
		Log("id = %d, name = %s", p->id, p->name.c_str());
	}
}

bool Scene::Intersect(const Ray& ray, float* tHit, SurfaceInteraction* isect) const {
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
