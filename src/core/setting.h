#ifndef CORE_SETTING_H
#define CORE_SETTING_H

#include<iostream>
#include<fstream>
#include<set>
#include"../../third_party/json/json.hpp"

using namespace nlohmann;

static std::set<std::string> valid_setting_key{
	"camera_look",
	"camera_pos",
	"camera_up",
	"camera_asp",
	"camera_fov",
	"camera_near",
	"camera_far",
	"camera_resX",
	"camera_resY",
	"ray_sample_no"
	"ray_bounce_no"
	"render_threads_count"
};

class Setting {
public:
	Setting(){
		//if(!LoadFile())
		//std::cout << "Setting.Setting()" << std::endl;
		LoadDefaultSetting();
		LoadFile();
	}

	auto Get(std::string key){
		return j[key];
	}

	template <typename T>
	int Set(std::string key, T v) {
		j[key] = v;
		return 0;
	}

	void LoadDefaultSetting() {
		std::cout << "Setting.LoadDefaultSetting()" << std::endl;

		j = json();
		j["camera_resX"] = 768;
		j["camera_resY"] = 768;
		j["camera_fov"] = 90.0;
		j["camera_asp"] = 1.0;
		j["camera_near"] = 0.1;
		j["camera_far"] = 1000.0;

		j["camera_pos"] = { 0.0, -20.0, 0.0 };
		j["camera_look"] = { 0.0, 0.0, 0.0 };
		j["camera_up"] = { 0.0, 0.0, 1.0 };

		j["ray_sample_no"] = 1;
		j["ray_bounce_no"] = 1;

		j["render_threads_count"] = 3;
	}

	int LoadFile() {
		std::cout << "Setting.LoadFile()" << std::endl;

		std::fstream f;
		f.open("cranks_renderer_setting.json", std::fstream::in | std::fstream::out);

		if (!f.is_open())
			return 0;

		f.seekp(0);

		json file_json;

		try {
			f >> file_json;

			for (auto& item : file_json.items()) {
				// std::cout  << item.key() << " " << item.value() << std::endl;
				if (valid_setting_key.find(item.key()) != valid_setting_key.end()) {
					j[item.key()] = item.value();
				}
			}
		}
		catch (json::type_error& e) {
			std::cout << e.what() << std::endl << "exception id: " << e.id << std::endl;
		}
		catch (...) {
			std::cout << "Setting.LoadFile() exception" << std::endl;
		}

		return 0;
	}

	int SaveFile() {
		std::ofstream f("cranks_renderer_setting.json", std::fstream::trunc);
		
		if (f.is_open()) {
			f << std::setw(4) << j << std::endl;

			std::cout << "Setting.SaveFile() ok" << std::endl;
			return 1;
		}

		return 0;
	}

	std::string Dump() {
		return j.dump();
	}

	json j;
};

// https://en.cppreference.com/w/cpp/language/inline
// https://stackoverflow.com/questions/14349877/static-global-variables-in-c
inline Setting setting;

#endif