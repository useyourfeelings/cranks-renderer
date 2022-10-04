#include "scene.h"
#include "../tool/logger.h"
#include "../tool/model.h"
#include "../shape/sphere.h"
#include "../shape/triangle.h"
#include "../light/point.h"
#include "../light/infinite.h"
#include "material.h"
#include "primitive.h"

Scene::Scene(std::shared_ptr<std::map<int, std::shared_ptr<Material>>> material_list):
	material_list(material_list),
	nodes_structure(1),
	latest_obj_id(0) {

	InitSceneTree();
}

int Scene::SetNodesStructure(int structure) {
	if (structure == 0 || structure == 1) {
		std::cout<<"SetNodesStructure "<< structure << std::endl;
		nodes_structure = structure;
		return 0;
	}

	return -1;
}

/*
void Scene::AddLight(std::shared_ptr<Light> light, const std::string& name) {
	lights.push_back(light);
}

void Scene::AddInfiniteLight(std::shared_ptr<Light> light, const std::string& name) {
	lights.push_back(light);
	infiniteLights.push_back(light);
}

void Scene::AddPrimitive(std::shared_ptr<Primitive> p) {
	//p->SetId(++ latest_obj_id);
	//p->SetName(name);
	//Log("AddPrimitive %d %s", latest_obj_id, p->Name().c_str());
	primitives.push_back(p);
}
*/

void Scene::InitBVH() {
	if (bvh == nullptr) {
		bvh = std::make_shared<BVH>(primitives);
	}
	
}

void Scene::RebuildBVH() {
	if (bvh == nullptr) {
		bvh = std::make_shared<BVH>(primitives);
	}
	else {
		bvh.reset(new BVH(primitives));
	}

}

void Scene::PrintScene() {
	Log("PrintScene");
	Log("latest_obj_id %d", latest_obj_id);
	for (auto p : primitives) {
		//Log("id = %d, name = %s", p->Id(), p->Name().c_str());
	}
}

const json& Scene::GetSceneTree() {
	std::cout << "Scene::GetSceneTree() "<<sceneTree << std::endl;
	return sceneTree;
}

int Scene::InitSceneTree() {
	sceneTree = json();

	/*sceneTree["0"] = {
		{"tree_path", {}},
		{"name", "root"},
		{"type", "folder"},
		{"children", json::object()}
	};*/

	json root_folder = json({
		{"tree_path", {}},
		{"name", "root"},
		{"type", "folder"},
		//{"children", json::object()}
		});

	AddObject(root_folder);

	return 0;
}

std::tuple<int, json> Scene::AddObject(const json& obj_info) {
	std::cout << "AddObject " << obj_info << std::endl;
	auto name = obj_info["name"];
	auto type = obj_info["type"];
	auto tree_path = obj_info["tree_path"]; // path of parent

	int obj_id = -1;

	json new_obj_info = obj_info;

	if (type == "folder") {
		auto obj = std::make_shared<Object>(name);
		obj->SetSceneID(++latest_obj_id);
		obj_id = obj->Id();

		id_obj_map[obj_id] = obj;

		auto node = &sceneTree;
		for (int node_id : tree_path) {
			node = &(*node)[std::to_string(node_id)]["children"];
		}

		new_obj_info["id"] = obj_id;
		new_obj_info["name"] = obj->Name();
		new_obj_info["children"] = json::object();

		(*node)[std::to_string(obj_id)] = new_obj_info;

		id_node_map[obj_id] = new_obj_info;

		//std::cout << "after add " << sceneTree << std::endl;

		return { 0, new_obj_info };
	}

	auto world_pos = obj_info["world_pos"];

	if (type == "sphere") {
		float radius = obj_info["radius"];

		auto t = Translate(Vector3f(world_pos[0], world_pos[1], world_pos[2]));

		float zmin = -radius;
		float zmax = radius;
		float phimax = 360.f;

		std::shared_ptr<Shape> shape = std::make_shared<Sphere>(t, Inverse(t), radius, zmin, zmax, phimax);

		std::shared_ptr<Material> material = nullptr;
		if (auto ml = material_list.lock()){
			material = ml->at(int(obj_info["material_id"]));
			std::cout << "AddObject get material " << material->GetID() << std::endl;
		}

		auto obj = std::make_shared<BasicModel>(name, t, material, shared_from_this());

		obj->SetSceneID(++latest_obj_id);
		obj_id = obj->Id();
		id_obj_map[obj_id] = obj;

		//auto obj = std::make_shared<GeometricPrimitive>(name, shape, GenMaterial(obj_info["material"]));
		auto geo = std::make_shared<GeometricPrimitive>(name, shape, obj);
		//AddPrimitive(std::make_shared<GeometricPrimitive>(name, shape, GenMaterial(material)));
		primitives.push_back(geo);

		auto node = &sceneTree;

		for (int node_id : tree_path) {
			node = &(* node)[std::to_string(node_id)]["children"];
		}

		//tree_path.push_back(obj->Id());
		
		new_obj_info["id"] = obj_id;
		new_obj_info["name"] = obj->Name();

		(*node)[std::to_string(obj_id)] = new_obj_info;
		id_node_map[obj_id] = new_obj_info;

		RebuildBVH();
	}
	else if (type == "mesh") {
		auto t = Translate(Vector3f(world_pos[0], world_pos[1], world_pos[2]));

		int tri_count, vertex_count;
		int* vertex_index;
		float* points;

		LoadGLTF(obj_info["file_name"], &tri_count, &vertex_count, &vertex_index, &points);

		//auto material = GenMaterial(obj_info["material"]);
		//auto material = material_list.at(int(obj_info["material_id"]));

		std::shared_ptr<Material> material = nullptr;
		if (auto ml = material_list.lock()) {
			material = ml->at(int(obj_info["material_id"]));
			std::cout << "AddObject get material " << material->GetID() << std::endl;
		}

		//auto obj = std::make_shared<TriangleMesh>(name, t, tri_count, vertex_index, vertex_count, points, obj_info["material_id"], nullptr);
		auto obj = std::make_shared<TriangleMesh>(name, t, tri_count, vertex_index, vertex_count, points, material, shared_from_this());

		delete[] vertex_index;
		delete[] points;

		for (int i = 0; i < tri_count; ++i) {
			std::shared_ptr<Shape> shape = std::make_shared<Triangle>(t, Inverse(t), obj, i);
			//AddPrimitive(std::make_shared<GeometricPrimitive>("NoName", shape, GenMaterial(material)));
			//primitives.push_back(std::make_shared<GeometricPrimitive>("NoName", shape, GenMaterial(obj_info["material"])));
			primitives.push_back(std::make_shared<GeometricPrimitive>("NoName", shape, obj));
		}

		obj->SetSceneID(++latest_obj_id);
		obj_id = obj->Id();
		std::cout << "obj_id = " << obj_id << std::endl;
		//primitives.push_back(obj);
		id_obj_map[obj_id] = obj;

		auto node = &sceneTree;

		for (int node_id : tree_path) {
			node = &(*node)[std::to_string(node_id)]["children"];
		}

		//tree_path.push_back(obj->Id());

		new_obj_info["id"] = obj_id;
		new_obj_info["name"] = obj->Name();

		(*node)[std::to_string(obj_id)] = new_obj_info;
		id_node_map[obj_id] = new_obj_info;

		RebuildBVH();
	}
	else if (type == "point_light") {
		auto t = Translate(Vector3f(world_pos[0], world_pos[1], world_pos[2]));

		auto obj = std::make_shared<PointLight>(name, t, Spectrum(obj_info["power"][0], obj_info["power"][1], obj_info["power"][2]));

		obj->SetSceneID(++latest_obj_id);
		obj_id = obj->Id();
		id_obj_map[obj_id] = obj;

		lights.push_back(obj);

		auto node = &sceneTree;

		for (int node_id : tree_path) {
			node = &(*node)[std::to_string(node_id)]["children"];
		}

		//tree_path.push_back(obj->Id());

		new_obj_info["id"] = obj_id;
		new_obj_info["name"] = obj->Name();

		(*node)[std::to_string(obj_id)] = new_obj_info;
		id_node_map[obj_id] = new_obj_info;
	}
	else if (type == "infinite_light") {
		auto t = Translate(Vector3f(world_pos[0], world_pos[1], world_pos[2]));

		auto obj = std::make_shared<InfiniteAreaLight>(name, t, 
			Spectrum(obj_info["power"][0], obj_info["power"][1], obj_info["power"][2]),
			obj_info["strength"], 
			obj_info["samples"],
			obj_info["file_name"]);

		obj->SetSceneID(++latest_obj_id);
		obj_id = obj->Id();
		infiniteLights.push_back(obj);
		id_obj_map[obj_id] = obj;

		auto node = &sceneTree;

		for (int node_id : tree_path) {
			node = &(*node)[std::to_string(node_id)]["children"];
		}

		//tree_path.push_back(obj->Id());

		new_obj_info["id"] = obj_id;
		new_obj_info["name"] = obj->Name();

		(*node)[std::to_string(obj_id)] = new_obj_info;
		id_node_map[obj_id] = new_obj_info;
	}

	//std::cout << "after add " << sceneTree << std::endl;

	return { 0, new_obj_info };
}

int Scene::DeleteObject(const json& obj_info) {
	std::cout << "DeleteObject " << obj_info << std::endl;

	if (obj_info["id"] == 1)
		return 0; // keep root

	auto node = &sceneTree;
	//auto tree_path = obj_info["tree_path"];
	//tree_path

	for (int node_id : obj_info["tree_path"]) {
		node = &(*node)[std::to_string(node_id)]["children"];
	}
	int scene_id = obj_info["id"];
	node->erase(std::to_string(scene_id));

	//id_obj_map.erase(scene_id);

	// 目前先一律重新加载
	json new_scene_tree = sceneTree;
	Reload(new_scene_tree);
	RebuildBVH();

	return 0;
}

int Scene::UpdateObject(const json& obj_info) {
	std::cout << "UpdateObject " << obj_info << std::endl;
	int node_id = obj_info["id"];
	
	auto node = &sceneTree;
	for (int node_id : id_node_map[node_id]["tree_path"]) {
		node = &(*node)[std::to_string(node_id)]["children"];
	}
	node = &(*node)[std::to_string(node_id)];
	node->update(obj_info);

	// 目前先一律重新加载
	json new_scene_tree = sceneTree;
	Reload(new_scene_tree);
	RebuildBVH();

	return 0;
}

std::string Scene::RenameObject(int node_id, const std::string &new_name) {
	std::cout << "RenameObject "<< node_id <<" " << new_name << std::endl;
	//int node_id = obj_info["id"];

	auto final_name = id_obj_map[node_id]->Rename(new_name);

	auto node = &sceneTree;
	for (int node_id : id_node_map[node_id]["tree_path"]) {
		node = &(*node)[std::to_string(node_id)]["children"];
	}
	node = &(*node)[std::to_string(node_id)];
	node->update(json({ {"name", final_name } }));

	return final_name;

	// 目前先一律重新加载
	//json new_scene_tree = sceneTree;
	//Reload(new_scene_tree);
	//RebuildBVH();

	//return 0;
}

std::shared_ptr<Material> Scene::GetMaterial(int material_id) {
	if (auto ml = material_list.lock()) {
		if (ml->contains(material_id))
			return ml->at(material_id);
		else
			return ml->at(1); // return default material. todo
	}
	else {
		throw("material_list gone");//return nullptr;
	}
}

int Scene::Reload(const json& load_scene_tree) {
	std::cout << "Scene::Reload 1" << std::endl;
	lights.clear();
	infiniteLights.clear();
	primitives.clear();
	id_obj_map.clear();
	id_node_map.clear();
	bvh.reset();
	latest_obj_id = 0;
	sceneTree = json();

	std::cout << "Scene::Reload 2" << std::endl;

	int count = 0;

	const auto load_obj = [&](const auto& myself, const json& parent, json tree_path) -> void {
		//std::cout << parent << std::endl;
		//std::cout << tree_path << std::endl;
		for (auto& [node_id, node] : parent.items()) {
			json new_node = node;
			new_node.erase("children");

			if (node["name"] == "root")
				new_node["tree_path"] = {};
			else {
				new_node["tree_path"] = tree_path;
			}
			
			AddObject(new_node);
			
			if (new_node["type"] == "folder") {
				tree_path.push_back(latest_obj_id);
				myself(myself, node["children"], tree_path);
			}
		}
	};

	load_obj(load_obj, load_scene_tree, json({0}));

	//std::cout << "reload " << count << " objects\n";
	
	return 0;
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
	/*for (auto p : primitives) {
		if (p->Id() == except_id)
			continue;

		if (p->Intersect(ray))
			return false;
	}*/

	return true;
}
