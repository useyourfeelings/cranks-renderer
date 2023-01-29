#ifndef CORE_OBJECT_H
#define CORE_OBJECT_H

/*
                                 Object         
	



*/

#include <iostream>
#include <string>
#include <format>
#include <unordered_set>
#include "../tool/json.h"
//#include "material.h"

class Material;
class Medium;

enum ObjectType {
	ObjectTypeFolder = 0,
	ObjectTypeScene,
	ObjectTypeMaterial,
	ObjectTypeMedium,

	ObjectTypeCount
};

// inline std::vector<std::unordered_set<std::string>> objects_name_sets(int(ObjectTypeCount));
inline std::unordered_set<std::string> objects_name_sets[int(ObjectTypeCount)];

// inline std::vector<int> latest_obj_ids;
inline int latest_obj_id = 0;

class Object { // : public std::enable_shared_from_this<Object> {
public:
	Object() {}

	Object(const json& new_config, ObjectType new_obj_type):
		obj_type(int(new_obj_type))
	{
		std::cout << "Object::Object() new_config " << new_config << " obj_type = " << obj_type << std::endl;
		//std::cout << std::format("Object::Object() new_config = {} obj_type = {}\n", new_config, new_obj_type);
		config = new_config;

		std::string new_name = new_config["name"];
		

		name = new_name;

		if (new_config.contains("id")) {
			obj_id = config["id"];
		} else {
			obj_id = ++latest_obj_id;
			config["id"] = obj_id;
		}

		if (name == "NoName")
			return;

		int index = 2;
		while (objects_name_sets[obj_type].contains(name)) {
			name = new_name + std::format("_{}", index++);
		}

		config["name"] = name;

		objects_name_sets[obj_type].insert(name);

		std::cout << "Object:Object ok " << config << std::endl;
	};

	virtual ~Object() {
		if (name != "NoName") {
			std::cout << "~Object() count = " << objects_name_sets[obj_type].size() << " erase " << name << std::endl;
			objects_name_sets[obj_type].erase(name);
		}
	};

	std::string Rename(const std::string& new_name) {
		std::cout << "Object.Rename() name = " << name << " new_name = " << new_name << std::endl;
		objects_name_sets[obj_type].erase(name);

		name = new_name;

		int index = 2;
		while (objects_name_sets[obj_type].contains(name)) {
			name = new_name + std::format("_{}", index++);
		}

		objects_name_sets[obj_type].insert(name);
		config["name"] = name;

		return name;
	}

	virtual std::shared_ptr<Material> GetMaterial() {
		return nullptr;
	}

	virtual std::shared_ptr<Medium> GetMedium() {
		return nullptr;
	}

	std::string Name() const {
		return name;
	}

	int Id() const {
		return obj_id;// scene_id;
	}

	int SetSceneID(int new_scene_id) {
		return scene_id = new_scene_id;
	}

	const json& GetJson() const {
		return config;
	}

	int GetID() const {
		return obj_id; // return config["id"];
	}

	json config;

	static void LoadLatestObjectID(int new_latest_obj_id) {
		latest_obj_id = new_latest_obj_id;
	}

	static int GetLatestObjectID() {
		return latest_obj_id;
	}

private:
	// 保存项目时是否要保存id？
	// 目前做成不保存id。加载scene等操作都重新生成id。

	int obj_type;

	int obj_id;
	int scene_id;
	std::string name;

};






#endif