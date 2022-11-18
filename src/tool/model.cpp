#include "model.h"
#include "json.h"
#include<iostream>
#include<fstream>

extern "C" {
	#include "../../third_party/base64/base64.h"
}

// load first model
int LoadGLTF(const std::string& file_name, int* tri_count, int* vertex_count, int** vertex_index, float** points) {
	std::fstream f;
	f.open(file_name, std::fstream::in);

	if (!f.is_open())
		return 0;

	f.seekp(0);

	json file_json;

	try {
		f >> file_json;
	}
	catch (json::type_error& e) {
		std::cout << e.what() << std::endl << "exception id: " << e.id << std::endl;
	}
	catch (...) {
		std::cout << "LoadGLTF() exception" << std::endl;
	}

	f.close();
	
	//std::cout << file_json["buffers"][0]["uri"] << std::endl;
	
	//for (auto& item : file_json["accessors"]) {
	//	if(item["bufferView"] == )
	//	// std::cout  << item.key() << " " << item.value() << std::endl;
	//	if (valid_setting_key.find(item.key()) != valid_setting_key.end()) {
	//		j[item.key()] = item.value();
	//	}
	//}

	try {
		auto mesh_name = file_json["meshes"][0]["name"];

		int pos_id = file_json["meshes"][0]["primitives"][0]["attributes"]["POSITION"];
		int pos_count = file_json["accessors"][pos_id]["count"];
		int pos_buffer_view_id = file_json["accessors"][pos_id]["bufferView"];
		int pos_buffer_offset = file_json["bufferViews"][pos_buffer_view_id]["byteOffset"];

		int buffer_id = file_json["bufferViews"][pos_buffer_view_id]["buffer"];

		auto uri_data = file_json["buffers"][buffer_id]["uri"];

		//std::cout << uri_data << std::endl;
		std::string uri_string(uri_data);
		//std::cout << uri_string << std::endl;

		int out_len;

		auto comma_index = uri_string.find_first_of(",");

		//std::cout << uri_string.c_str() + comma_index + 1 << std::endl;

		unsigned char* mesh_bytes = unbase64(uri_string.c_str() + comma_index + 1, int(uri_string.length() - comma_index - 1), &out_len);

		std::cout << out_len << std::endl;
		std::cout << mesh_bytes << std::endl;

		*vertex_count = pos_count;

		*points = new float[pos_count * 3];

		for (int i = 0; i < pos_count; ++i) {
			int offset = pos_buffer_offset + i * 12;
			memcpy(&(*points)[i * 3], &mesh_bytes[offset], 4); // x
			memcpy(&(*points)[i * 3 + 1], &mesh_bytes[offset + 4], 4); // y
			memcpy(&(*points)[i * 3 + 2], &mesh_bytes[offset + 8], 4); // z
		}

		int index_id = file_json["meshes"][0]["primitives"][0]["indices"];
		int index_count = file_json["accessors"][index_id]["count"];
		int index_buffer_view_id = file_json["accessors"][index_id]["bufferView"];
		int index_buffer_offset = file_json["bufferViews"][index_buffer_view_id]["byteOffset"];

		*tri_count = index_count / 3;

		*vertex_index = new int[index_count];

		for (int i = 0; i < index_count; ++i) {
			//memcpy(&(*vertex_index)[i], &mesh_bytes[index_buffer_offset + 2 * i], 2);
			(* vertex_index)[i] =
				0 |
				mesh_bytes[index_buffer_offset + 2 * i + 1] << 8 |
				mesh_bytes[index_buffer_offset + 2 * i];
		}

		std::cout << "LoadGLTF over vertex_count "<< pos_count <<" tri_count "<< index_count / 3 << std::endl;

	}
	catch (json::type_error& e) {
		std::cout << e.what() << std::endl << "exception id: " << e.id << std::endl;
		return -1;
	}
	catch (...) {
		std::cout << "LoadGLTF() exception" << std::endl;
		return -1;
	}

	return 0;
}
