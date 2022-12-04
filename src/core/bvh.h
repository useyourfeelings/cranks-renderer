#ifndef CORE_BVH_H
#define CORE_BVH_H

#include <vector>
#include <memory>
#include <string>
#include "geometry.h"
#include "interaction.h"

// todo-node的更新/添加/删除

struct BVHBin {
	size_t count = 0;
	BBox3f bounds;
};

class BVHNode {
public:
	BVHNode():id(0), 
		index(0),
		count(0),
		left(nullptr),
		right(nullptr)
	{	};

	// 做叶子
	void MakeLeaf(size_t id, size_t index, size_t count, const BBox3f& b) {
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
	void MakeInterior(size_t id, BVHNode* l, BVHNode* r) {
		//std::cout <<"MakeInterior id "<<id << " l " << l << " r " << r <<" l->id "<<l->id << std::endl;
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

	size_t id; // 调试用

	// 生成nodes时，三角形按顺序插入orderedPrims。
	// index为这个node里的第一个三角形在orderedPrims的index。
	size_t index;

	size_t count; // 叶子可存多个三角形

	BBox3f bounds;
	BVHNode* left;
	BVHNode* right;
};

class BVHPrimitiveInfo {
public:
	BVHPrimitiveInfo():pIndex(0) {};
	BVHPrimitiveInfo(size_t pIndex, const BBox3f& bounds) :
		pIndex(pIndex),
		bounds(bounds),
		centroid((bounds.pMin + bounds.pMax) * 0.5) {
	}

	// 对应原始primitives的index
	// primitiveInfo和primitives初始状态的index是一致的。
	// 但是途中partition后，primitiveInfo的顺序会打乱。
	size_t pIndex;

	BBox3f bounds;
	Point3f centroid;
};

struct BVHBuildTask {
	char type; // B-build M-MakeInterior N-node_id
	size_t start;
	size_t end;
	size_t node_id;
	//size_t node_id_l;
	//size_t node_id_r;
};

class BVH {
public:
	BVH(const std::vector<std::shared_ptr<Primitive>>& p);
	bool Intersect(const Ray& ray, float* tHit, SurfaceInteraction* isect, int pool_id);
	bool Intersect(const Ray& ray) const;
	void Print() const;
	~BVH() {
		for (int i = 0; i < 32; ++i) {
			if (hopes_pool[i] != nullptr)
				delete[] hopes_pool[i];
		}
	}

private:
	void Build(std::vector<BVHPrimitiveInfo>& primitiveInfo, std::vector<std::shared_ptr<Primitive>>& orderedPrims);

	void InternalBuild(std::vector<BVHPrimitiveInfo>& primitiveInfo, size_t start, size_t end,
		std::vector<std::shared_ptr<Primitive>>& orderedPrims, std::vector<BVHBuildTask>& task_q);

	void PrintNode(size_t node_id, size_t parent_id) const;

	std::vector<std::shared_ptr<Primitive>> primitives;

	std::vector<BVHNode> tree;

	const int binCount = 12;

	// multi thread

	// 多线程使用vector的话严重降速
	//std::vector<int> hopes_status;
	//std::vector<std::vector<const BVHNode*>> hopes_pool; // max 32
	//std::vector<const BVHNode*> hopes_pool[32]; // max 32
	
	std::mutex hopes_mutex;
	BVHNode** hopes_pool[32]; // max 32
	int hopes_status[32]; // 0-void 1-ready 2-inuse

	int GetHopesPool() { // dynamic. get pool
		hopes_mutex.lock();

		int pool_id = -1;

		for (int i = 0; i < 32; ++i) {
			if (hopes_status[i] == 0) {
				hopes_pool[i] = new BVHNode*[tree.size()];
				hopes_status[i] = 2;
				pool_id = i;
				break;
			} else if (hopes_status[i] == 1) {
				hopes_status[i] = 2;
				pool_id = i;
				break;
			}
		}

		//if (pool_id == -1) { // poll full
		//	hopes_status.push_back(1);
		//	hopes_pool.push_back(std::vector<const BVHNode*>());
		//	hopes_pool.back().reserve(tree.size() / 3);
		//	pool_id = int(hopes_pool.size()) - 1;
		//}

		hopes_mutex.unlock();

		return pool_id;
	}

	void ReleaseHopesPool(int pool_id) {
		hopes_mutex.lock();
		hopes_status[pool_id] = 1;
		hopes_mutex.unlock();
	}
};


#endif