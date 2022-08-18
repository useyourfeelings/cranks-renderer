#include "bvh.h"

BVH::BVH(const std::vector<std::shared_ptr<Primitive>> &p):
	primitives(std::move(p)),
	binCount(12) {

	// 初始化primitiveInfo
	std::vector<BVHPrimitiveInfo> primitiveInfo(primitives.size());
	for (int i = 0; i < primitives.size(); ++i) {
		primitiveInfo[i] = { i, primitives[i]->WorldBound() };

		/*std::cout << std::format("wtf [{}, {}, {}] [{}, {}, {}]\n",
			primitiveInfo[i].bounds.pMin.x, primitiveInfo[i].bounds.pMin.y, primitiveInfo[i].bounds.pMin.z,
			primitiveInfo[i].bounds.pMax.x, primitiveInfo[i].bounds.pMax.y, primitiveInfo[i].bounds.pMax.z);*/
	}

	int totalNodes = 0;

	// orderedPrims用来存顺序插入的三角形
	// 有时一个集合中所有质点相等。需要把其中所有三角形做到一个叶子里。
	// 那么维持好添加的顺序，后续可以更好地取三角形。
	std::vector<std::shared_ptr<Primitive>> orderedPrims;
	orderedPrims.reserve(primitives.size());

	root = Build(primitiveInfo, 0, primitives.size() - 1, &totalNodes, orderedPrims);

	primitives.swap(orderedPrims); // 替换为按顺序排列

	std::cout << std::format("BVH build ok totalNodes = {}\n", totalNodes);

	//Print();
}

std::shared_ptr<BVHNode> BVH::Build(std::vector<BVHPrimitiveInfo>& primitiveInfo, int start, int end, int* totalNodes,
	std::vector<std::shared_ptr<Primitive>>& orderedPrims) {

	//std::cout << std::format("Build {} {} current nodes {} \n", start, end, *totalNodes);

	auto node = std::make_shared<BVHNode>();
	(* totalNodes)++;
	int node_id = *totalNodes;

	if (start == end) {
		/*std::cout << std::format("before MakeLeaf [{}, {}, {}] [{}, {}, {}]\n",
			primitiveInfo[start].bounds.pMin.x, primitiveInfo[start].bounds.pMin.y, primitiveInfo[start].bounds.pMin.z,
			primitiveInfo[start].bounds.pMax.x, primitiveInfo[start].bounds.pMax.y, primitiveInfo[start].bounds.pMax.z);

		std::cout << std::format("{}\n", primitives[primitiveInfo[start].pIndex]->GetInfoString());*/

		// 只有一个了。直接做叶子。
		node->MakeLeaf(node_id, orderedPrims.size(), 1, primitiveInfo[start].bounds);
		orderedPrims.push_back(primitives[primitiveInfo[start].pIndex]);
		return node;
	}

	// 质点总box
	BBox3f centroidBounds;

	for (int i = start; i <= end; ++i) {
		centroidBounds = Union(centroidBounds, primitiveInfo[i].centroid);
	}

	// 按质点bbox选轴
	auto diagonal = centroidBounds.pMax - centroidBounds.pMin;

	int axis;
	if (diagonal.x > diagonal.y && diagonal.x > diagonal.z)
		axis = 0;
	else if (diagonal.y > diagonal.z)
		axis = 1;
	else
		axis = 2;

	if (centroidBounds.pMax[axis] == centroidBounds.pMin[axis]) {
		// 质点相等放。做到同个叶子里。
		BBox3f pBounds;
		int index = orderedPrims.size();

		for (int i = start; i <= end; ++i) {
			orderedPrims.push_back(primitives[primitiveInfo[i].pIndex]);
			/*std::cout << std::format("before {} MakeLeaf [{}, {}, {}] [{}, {}, {}]\n", i,
				primitiveInfo[i].bounds.pMin.x, primitiveInfo[i].bounds.pMin.y, primitiveInfo[i].bounds.pMin.z,
				primitiveInfo[i].bounds.pMax.x, primitiveInfo[i].bounds.pMax.y, primitiveInfo[i].bounds.pMax.z);

			std::cout << std::format("{}\n", primitives[primitiveInfo[i].pIndex]->GetInfoString());*/

			pBounds = Union(pBounds, primitiveInfo[i].bounds);
		}

		node->MakeLeaf(node_id, index, end - start + 1, pBounds);
		return node;
	}

	// 生成bin信息
	auto bins = std::vector<BVHBin>(binCount);
	int bin;

	for (int i = start; i <= end; ++i) {

		bin = binCount * (0.0001f + primitiveInfo[i].centroid[axis] - centroidBounds.pMin[axis]) /
			(centroidBounds.pMax[axis] - centroidBounds.pMin[axis]);

		if (bin > binCount - 1)
			bin = binCount - 1;

		//std::cout << std::format("i {} bin {} {} {} {} ", i, bin, primitiveInfo[i].centroid[axis], centroidBounds.pMin[axis], centroidBounds.pMax[axis]) << std::endl;

		bins[bin].count++;
		bins[bin].bounds.Union(primitiveInfo[i].bounds);
	}

	// 遍历bins。算sah。
	// 考虑极端情况。比如有的bin为空。
	// 全在一个bin好像不可能？最起码一头一尾。
	int countL = 0;
	BBox3f bboxL;
	float bestCost = MaxFloat;
	int bestBin;

	for (int i = 0; i < bins.size() - 1; ++i) {
		if (bins[i].count == 0) // 空的跳过
			continue;

		// 从i右边切开
		// 左边的值
		countL += bins[i].count;
		bboxL.Union(bins[i].bounds);

		// 右边的值
		int countR = 0;
		BBox3f bboxR;
		for (int j = i + 1; j < bins.size(); ++ j) {
			countR += bins[j].count;
			bboxR.Union(bins[j].bounds);
		}
		
		// sah
		float cost = countL * bboxL.SurfaceArea() + countR * bboxR.SurfaceArea();

		if (cost < bestCost) {
			bestCost = cost;
			bestBin = i;
		}
	}

	// 根据bestBin分成两组。继续往下算。
	auto pmid = std::partition(&primitiveInfo[start], &primitiveInfo[end] + 1,
		[=](const BVHPrimitiveInfo& i) {
			int bin = binCount * (0.0001f + i.centroid[axis] - centroidBounds.pMin[axis]) /
				(centroidBounds.pMax[axis] - centroidBounds.pMin[axis]);

			if (bin > binCount - 1)
				bin = binCount - 1;

			return bin <= bestBin;
		}
	);

	int mid = pmid - &primitiveInfo[0];

	//std::cout << "mid = " << mid << std::endl;

	node->MakeInterior(node_id,
		Build(primitiveInfo, start, mid - 1, totalNodes, orderedPrims),
		Build(primitiveInfo, mid, end, totalNodes, orderedPrims));

	return node;
}

bool BVH::Intersect(const Ray& ray) const {
	Vector3f invDir(1 / ray.d.x, 1 / ray.d.y, 1 / ray.d.z);
	int dirIsNeg[3] = { invDir.x < 0, invDir.y < 0, invDir.z < 0 };

	std::vector<std::shared_ptr<BVHNode>> hopes;
	hopes.push_back(root);

	float t;

	// 做成后进先出。深度优先。

	while (true) {
		if (hopes.empty()) // 所有可能用尽
			return false;

		auto hope = hopes.back();
		hopes.pop_back();

		// 如果bbox相交
		if (hope->bounds.Intersect(ray, invDir, dirIsNeg, &t)) {
			// 如果是叶子
			if (hope->left == nullptr && hope->right == nullptr) {
				// primitive确实相交
				for (int i = 0; i < hope->count; ++i) {
					if (this->primitives[hope->index + i]->Intersect(ray)) {
						return true;
					}
				}
			}
			else {
				// todo-判断方向。先算概率大的。
				hopes.push_back(hope->left);
				hopes.push_back(hope->right);
			}
		}
	}

}


bool BVH::Intersect(const Ray& ray, float *tHit, SurfaceInteraction* isect) const {
	Vector3f invDir(1 / ray.d.x, 1 / ray.d.y, 1 / ray.d.z);
	int dirIsNeg[3] = { invDir.x < 0, invDir.y < 0, invDir.z < 0 };

	float t;

	if (!root->bounds.Intersect(ray, invDir, dirIsNeg, &t))
		return false;

	std::vector<std::shared_ptr<BVHNode>> hopes;
	hopes.push_back(root);

	bool hit = false;
	float tMin = std::numeric_limits<float>::max();
	SurfaceInteraction tempIsect;

	// 做成后进先出。深度优先。进去的一定是有希望的bbox。

	while (true) {
		if (hopes.empty()) // 所有可能用尽
			break;

		auto hope = hopes.back();
		hopes.pop_back();

		//std::cout << std::format("node id {}\n", hope->id);
		
		// 如果是叶子
		if (hope->left == nullptr && hope->right == nullptr) {
			// primitive确实相交
			for (int i = 0; i < hope->count; ++i) {
				if (this->primitives[hope->index + i]->Intersect(ray, &t, &tempIsect)) {
					if (t < tMin) {
						hit = true;
						tMin = t;
						*tHit = t;
						*isect = tempIsect;
					}
				}
			}
		}
		else {
			// todo 判断方向

			// bbox求交的t>=tMin的话没必要再查。

			if (hope->left->bounds.Intersect(ray, invDir, dirIsNeg, &t)) {
				if (t < tMin) {
					hopes.push_back(hope->left);
				}
			}

			if (hope->right->bounds.Intersect(ray, invDir, dirIsNeg, &t)) {
				if (t < tMin) {
					hopes.push_back(hope->right);
				}
			}
			
		}
	}


	return hit;
}


void BVH::Print() const {
	std::cout << "BVH::Print()\n";
	PrintNode(root, 0);
}

void BVH::PrintNode(std::shared_ptr<BVHNode> node, int parent_id) const {
	std::cout << std::format("id {} pid {}, {}, {}, [{}, {}, {}] [{}, {}, {}]\n", node->id, parent_id, node->index, node->count,
		node->bounds.pMin.x, node->bounds.pMin.y, node->bounds.pMin.z,
		node->bounds.pMax.x, node->bounds.pMax.y, node->bounds.pMax.z);

	for (int i = 0; i < node->count; ++i) {
		std::cout << std::format("{}\n", primitives[node->index + i]->GetInfoString());
	}

	if (node->left != nullptr)
		PrintNode(node->left, node->id);

	if (node->right != nullptr)
		PrintNode(node->right, node->id);
}