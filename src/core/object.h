#ifndef CORE_OBJECT_H
#define CORE_OBJECT_H

/*
                                 Object         
	



*/

#include <iostream>
#include <string>
#include <format>
#include <unordered_set>
#include "material.h"

inline std::unordered_set<std::string> objects_name_set;
inline int latest_obj_id = 0;

class Object {
public:
	Object(const std::string& new_name) {
		std::cout << "Object::Object() new_name = " << new_name << std::endl;

		obj_id = ++latest_obj_id;
		name = new_name;

		if (new_name == "NoName")
			return;

		int index = 2;
		while (objects_name_set.contains(name)) {
			name = new_name + std::format("_{}", index++);
		}

		objects_name_set.insert(name);
	};

	virtual ~Object() {
		if (name != "NoName") {
			std::cout << "~Object() count = "<< objects_name_set.size() << " erase " << name << std::endl;
			objects_name_set.erase(name);
		}
			
	};

	std::string Rename(const std::string &new_name) {
		std::cout << "Object.Rename() name = " << name << " new_name = " << new_name << std::endl;
		objects_name_set.erase(name);

		name = new_name;

		int index = 2;
		while (objects_name_set.contains(name)) {
			name = new_name + std::format("_{}", index++);
		}

		objects_name_set.insert(name);

		return name;
	}

	virtual std::shared_ptr<Material> GetMaterial() {
		return nullptr;
	}

	std::string Name() const {
		return name;
	}

	int Id() const {
		return scene_id;
	}

	int SetSceneID(int new_scene_id) {
		return scene_id = new_scene_id;
	}

private:
	// 保存项目时是否要保存id？
	// 目前做成不保存id。加载scene等操作都重新生成id。

	int obj_id;
	int scene_id;
	std::string name;

	//float x, y, z;
};






#endif