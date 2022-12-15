#ifndef TOOL_MODEL_H
#define TOOL_MODEL_H

#include<string>

int LoadGLTF(const std::string& file_name, int * tri_count, int * vertex_count, int ** vertex_index, float ** points, float** normals);

#endif