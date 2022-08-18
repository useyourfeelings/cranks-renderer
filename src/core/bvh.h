#ifndef CORE_BVH_H
#define CORE_BVH_H

#include <vector>
#include <memory>
#include <string>
#include "geometry.h"
#include "interaction.h"
#include "primitive.h"

// todo-node的更新/添加/删除

struct BVHBin {
	int count = 0;
	BBox3f bounds;
};

class BVHNode {
public:
	BVHNode() {};

	// 做叶子
	void MakeLeaf(int id, int index, int count, const BBox3f& b) {
		this->id = id;
		this->index = index;
		this->count = count;
		bounds = b;
		left = right = nullptr;

		/*std::cout << std::format("MakeLeaf id {} index {} count {}, [{}, {}, {}] [{}, {}, {}]\n", id, index, count,
			bounds.pMin.x, bounds.pMin.y, bounds.pMin.z,
			bounds.pMax.x, bounds.pMax.y, bounds.pMax.z);*/
	}

	// 做内部节点
	void MakeInterior(int id, std::shared_ptr<BVHNode> l, std::shared_ptr<BVHNode> r) {
		this->id = id;
		this->index = 0;
		this->count = 0;
		left = l;
		right = r;
		bounds = Union(l->bounds, r->bounds);

		/*std::cout << std::format("MakeInterior id {} index {} count {}, [{}, {}, {}] [{}, {}, {}]\n", id, index, count,
			bounds.pMin.x, bounds.pMin.y, bounds.pMin.z,
			bounds.pMax.x, bounds.pMax.y, bounds.pMax.z);*/
	}

	int id; // 调试用

	// 生成nodes时，三角形按顺序插入orderedPrims。
	// index为这个node里的第一个三角形在orderedPrims的index。
	int index;

	int count; // 叶子可存多个三角形

	BBox3f bounds;
	std::shared_ptr<BVHNode> left;
	std::shared_ptr<BVHNode> right;
};

class BVHPrimitiveInfo {
public:
	BVHPrimitiveInfo() {};
	BVHPrimitiveInfo(int pIndex, const BBox3f& bounds):
		pIndex(pIndex),
		bounds(bounds),
		centroid((bounds.pMin + bounds.pMax) * 0.5) {
	}

	// 对应原始primitives的index
	// primitiveInfo和primitives初始状态的index是一致的。
	// 但是途中partition后，primitiveInfo的顺序会打乱。
	int pIndex;

	BBox3f bounds;
	Point3f centroid;
};


class BVH {
public:
	BVH(const std::vector<std::shared_ptr<Primitive>>& p);
	bool Intersect(const Ray& ray, float* tHit, SurfaceInteraction* isect) const;
	bool Intersect(const Ray& ray) const;
	void Print() const;

private:
	std::shared_ptr<BVHNode> Build(std::vector<BVHPrimitiveInfo>& primitiveInfo, int start, int end, int* totalNodes,
		std::vector<std::shared_ptr<Primitive>>& orderedPrims);

	void PrintNode(std::shared_ptr<BVHNode> node, int parent_id) const;

	std::vector<std::shared_ptr<Primitive>> primitives;

	std::shared_ptr<BVHNode> root;

	int binCount;
};


#endif