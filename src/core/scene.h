#ifndef CORE_SCENE_H
#define CORE_SCENE_H

#include <list>
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <tuple>
#include "object.h"
#include "geometry.h"
#include "interaction.h"
#include "light.h"
#include "bvh.h"
#include"../tool/json.h"

class Scene : public std::enable_shared_from_this<Scene> {
public:
	Scene(std::shared_ptr<std::map<int, std::shared_ptr<Material>>> material_list);

	bool Intersect(const Ray& ray, float* tHit, SurfaceInteraction* isect) const;
	bool Intersect(const Ray& ray, int except_id) const;

	void AddLight(std::shared_ptr<Light>, const std::string& name);
	void AddInfiniteLight(std::shared_ptr<Light> light, const std::string& name);
	void AddPrimitive(std::shared_ptr<Primitive>);
	std::tuple<int, json> AddObject(const json& obj_info);
	int DeleteObject(const json& obj_info);
	int UpdateObject(const json& obj_info);
	std::string RenameObject(int obj_id, const std::string& new_name);
	void PrintScene();
	void InitBVH();
	void RebuildBVH();

	int Reload(const json& scene_json);

	int SetNodesStructure(int structure);
	const json& GetSceneTree();

	int InitSceneTree();

	std::shared_ptr<Material> GetMaterial(int material_id);

	std::vector<std::shared_ptr<Light>> lights;
	std::vector<std::shared_ptr<Light>> infiniteLights;
	std::vector<std::shared_ptr<Primitive>> primitives;

	std::shared_ptr<BVH> bvh = nullptr;

	/*
	{
		1:{
			//"id":1
			"name":"root"
			"type":"folder"
			"children":{
				{ 2:
					{
						//"id":2
						"name":"ggg"
						"type":"object"
						"children":[]
					}
				}
				{...},
				{...},
				...
			}
		}
	}
	*/

private:
	std::unordered_map<int, std::shared_ptr<Object>> id_obj_map;

	std::unordered_map<int, json> id_node_map;

	json sceneTree; // list all objects

	std::weak_ptr<std::map<int, std::shared_ptr<Material>>> material_list;

	int nodes_structure; // 0-brute force 1-bvh
	int latest_obj_id;
	int latest_tree_node_id;
};


#endif