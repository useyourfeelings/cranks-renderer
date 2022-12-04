#include "bvh.h"
#include "primitive.h"

BVH::BVH(const std::vector<std::shared_ptr<Primitive>>& p) :
	primitives(p)
	//binCount(12) 
	{

	if (primitives.empty())
		return;

	// ��ʼ��primitiveInfo
	std::vector<BVHPrimitiveInfo> primitiveInfo(primitives.size());
	for (size_t i = 0; i < primitives.size(); ++i) {
		primitiveInfo[i] = { i, primitives[i]->WorldBound() };

		/*std::cout << std::format("wtf [{}, {}, {}] [{}, {}, {}]\n",
			primitiveInfo[i].bounds.pMin.x, primitiveInfo[i].bounds.pMin.y, primitiveInfo[i].bounds.pMin.z,
			primitiveInfo[i].bounds.pMax.x, primitiveInfo[i].bounds.pMax.y, primitiveInfo[i].bounds.pMax.z);*/
	}

	tree.reserve(primitives.size() * 2 - 1); // vectorԪ�ص�ַ��䡣������reserve�Ͳ���䡣���������ص����⡣

	// orderedPrims������˳������������
	// ��ʱһ�������������ʵ���ȡ���Ҫ��������������������һ��Ҷ���
	// ��ôά�ֺ���ӵ�˳�򣬺������Ը��õ�ȡ�����Ρ�
	std::vector<std::shared_ptr<Primitive>> orderedPrims;
	orderedPrims.reserve(primitives.size());

	Build(primitiveInfo, orderedPrims);

	primitives.swap(orderedPrims); // �滻Ϊ��˳������

	for (int i = 0; i < 32; ++i) {
		hopes_pool[i] = nullptr;
		hopes_status[i] = 0;
		hopes_pool[i] = new BVHNode * [tree.size()];
	}

	std::cout << std::format("BVH build ok totalNodes = {}\n", tree.size());

	//Print();
}

#if 0
size_t BVH::Build(std::vector<BVHPrimitiveInfo>& primitiveInfo, size_t start, size_t end,
	std::vector<std::shared_ptr<Primitive>>& orderedPrims) {

	size_t node_id = tree.size();

	//std::cout << std::format("Build {} {} current node_id {} \n", start, end, node_id);

	tree.push_back(BVHNode());

	if (start == end) {
		/*std::cout << std::format("before MakeLeaf [{}, {}, {}] [{}, {}, {}]\n",
			primitiveInfo[start].bounds.pMin.x, primitiveInfo[start].bounds.pMin.y, primitiveInfo[start].bounds.pMin.z,
			primitiveInfo[start].bounds.pMax.x, primitiveInfo[start].bounds.pMax.y, primitiveInfo[start].bounds.pMax.z);

		std::cout << std::format("{}\n", primitives[primitiveInfo[start].pIndex]->GetInfoString());*/

		// ֻ��һ���ˡ�ֱ����Ҷ�ӡ�
		tree[node_id].MakeLeaf(node_id, orderedPrims.size(), 1, primitiveInfo[start].bounds);
		orderedPrims.push_back(primitives[primitiveInfo[start].pIndex]);
		
		return node_id;
	}

	// �ʵ���box
	BBox3f centroidBounds;

	for (size_t i = start; i <= end; ++i) {
		centroidBounds = Union(centroidBounds, primitiveInfo[i].centroid);
	}

	// ���ʵ�bboxѡ��
	auto diagonal = centroidBounds.pMax - centroidBounds.pMin;

	int axis;
	if (diagonal.x > diagonal.y && diagonal.x > diagonal.z)
		axis = 0;
	else if (diagonal.y > diagonal.z)
		axis = 1;
	else
		axis = 2;

	if (centroidBounds.pMax[axis] == centroidBounds.pMin[axis]) {
		// �ʵ���ȷš�����ͬ��Ҷ���
		BBox3f pBounds;
		size_t index = orderedPrims.size();

		for (size_t i = start; i <= end; ++i) {
			orderedPrims.push_back(primitives[primitiveInfo[i].pIndex]);
			/*std::cout << std::format("before {} MakeLeaf [{}, {}, {}] [{}, {}, {}]\n", i,
				primitiveInfo[i].bounds.pMin.x, primitiveInfo[i].bounds.pMin.y, primitiveInfo[i].bounds.pMin.z,
				primitiveInfo[i].bounds.pMax.x, primitiveInfo[i].bounds.pMax.y, primitiveInfo[i].bounds.pMax.z);

			std::cout << std::format("{}\n", primitives[primitiveInfo[i].pIndex]->GetInfoString());*/

			pBounds = Union(pBounds, primitiveInfo[i].bounds);
		}

		tree[node_id].MakeLeaf(node_id, index, end - start + 1, pBounds);
		return node_id;
	}

	// ����bin��Ϣ
	//std::vector<BVHBin> bins(binCount);
	BVHBin bins[12];

	int bin;

	for (size_t i = start; i <= end; ++i) {

		bin = binCount * (0.0001f + primitiveInfo[i].centroid[axis] - centroidBounds.pMin[axis]) /
			(centroidBounds.pMax[axis] - centroidBounds.pMin[axis]);

		if (bin > binCount - 1)
			bin = binCount - 1;

		//std::cout << std::format("i {} bin {} {} {} {} ", i, bin, primitiveInfo[i].centroid[axis], centroidBounds.pMin[axis], centroidBounds.pMax[axis]) << std::endl;

		bins[bin].count++;
		bins[bin].bounds.Union(primitiveInfo[i].bounds);
	}

	// ����bins����sah��
	// ���Ǽ�������������е�binΪ�ա�
	// ȫ��һ��bin���񲻿��ܣ�������һͷһβ��
	size_t countL = 0;
	BBox3f bboxL;
	float bestCost = MaxFloat;
	size_t bestBin;

	for (size_t i = 0; i < binCount - 1; ++i) {
		if (bins[i].count == 0) // �յ�����
			continue;

		// ��i�ұ��п�
		// ��ߵ�ֵ
		countL += bins[i].count;
		bboxL.Union(bins[i].bounds);

		// �ұߵ�ֵ
		size_t countR = 0;
		BBox3f bboxR;
		for (size_t j = i + 1; j < binCount; ++j) {
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

	// ����bestBin�ֳ����顣���������㡣
	auto pmid = std::partition(&primitiveInfo[start], &primitiveInfo[end] + 1,
		[&](const BVHPrimitiveInfo& i) {
			int bin = binCount * (0.0001f + i.centroid[axis] - centroidBounds.pMin[axis]) /
				(centroidBounds.pMax[axis] - centroidBounds.pMin[axis]);

			if (bin > binCount - 1)
				bin = binCount - 1;

			return bin <= bestBin;
		}
	);

	int mid = pmid - &primitiveInfo[0];

	//std::cout << "mid = " << mid << std::endl;

	/*node->MakeInterior(node_id,
		Build(primitiveInfo, start, mid - 1, totalNodes, orderedPrims),
		Build(primitiveInfo, mid, end, totalNodes, orderedPrims));*/

	size_t l_id = Build(primitiveInfo, start, mid - 1, orderedPrims);
	size_t r_id = Build(primitiveInfo, mid, end, orderedPrims);

	//std::cout << "MakeInterior " << node_id << " l = " << l_id << " r = " << r_id << " "<< ( & tree[l_id])->id << " " << (&tree[r_id])->id << std::endl;
	tree[node_id].MakeInterior(node_id, &tree[l_id], &tree[r_id]);

	return node_id;
}
#endif

/*
struct BuildTask {
	char type; // B-build M-MakeInterior N-node_id
	size_t start;
	size_t end;
	size_t node_id;
	size_t node_id_l;
	size_t node_id_r;
};*/

void BVH::Build(std::vector<BVHPrimitiveInfo>& primitiveInfo, std::vector<std::shared_ptr<Primitive>>& orderedPrims) {

	std::vector<BVHBuildTask> task_q;
	task_q.reserve(primitives.size());

	BVHBuildTask task;
	task.type = 'B';
	task.start = 0;
	task.end = primitives.size() - 1;

	task_q.push_back(task);

	while (task_q.size()) {
		size_t size = task_q.size();
		auto task = task_q.back();
		if (task.type == 'B') {
			task_q.pop_back();
			InternalBuild(primitiveInfo, task.start, task.end, orderedPrims, task_q);
		} else if (task.type == 'N') {
			if (size == 1)
				break;

			if (task_q[size - 2].type == 'B')
				std::swap(task_q[size - 1], task_q[size - 2]); // swap BN
			else if (task_q[size - 2].type == 'N' && task_q[size - 3].type == 'M') { // MNN -> N
				auto node_id = task_q[size - 3].node_id;
				auto node_id_l = task_q[size - 1].node_id;
				auto node_id_r = task_q[size - 2].node_id;
				task_q.pop_back();
				task_q.pop_back();

				tree[node_id].MakeInterior(node_id, &tree[node_id_l], &tree[node_id_r]);
				task_q[size - 3].node_id = node_id;
				task_q[size - 3].type = 'N';
			}
			else {
				throw("bvh 1");
			}
			//task_q.pop_back();
			//InternalBuild(primitiveInfo, task.start, task.end, orderedPrims, task_q);
		}
		else {
			throw("bvh 2");
		}
	}
}

void BVH::InternalBuild(std::vector<BVHPrimitiveInfo>& primitiveInfo, size_t start, size_t end,
	std::vector<std::shared_ptr<Primitive>>& orderedPrims, std::vector<BVHBuildTask> &task_q) {

	size_t node_id = tree.size();

	//std::cout << std::format("Build {} {} current node_id {} \n", start, end, node_id);

	tree.push_back(BVHNode());

	if (start == end) {
		/*std::cout << std::format("before MakeLeaf [{}, {}, {}] [{}, {}, {}]\n",
			primitiveInfo[start].bounds.pMin.x, primitiveInfo[start].bounds.pMin.y, primitiveInfo[start].bounds.pMin.z,
			primitiveInfo[start].bounds.pMax.x, primitiveInfo[start].bounds.pMax.y, primitiveInfo[start].bounds.pMax.z);

		std::cout << std::format("{}\n", primitives[primitiveInfo[start].pIndex]->GetInfoString());*/

		// ֻ��һ���ˡ�ֱ����Ҷ�ӡ�
		tree[node_id].MakeLeaf(node_id, orderedPrims.size(), 1, primitiveInfo[start].bounds);
		orderedPrims.push_back(primitives[primitiveInfo[start].pIndex]);

		// push N
		BVHBuildTask task;
		task.type = 'N';
		task.node_id = node_id;
		task_q.push_back(task);

		return;// node_id;
	}

	// �ʵ���box
	BBox3f centroidBounds;

	for (size_t i = start; i <= end; ++i) {
		centroidBounds = Union(centroidBounds, primitiveInfo[i].centroid);
	}

	// ���ʵ�bboxѡ��
	auto diagonal = centroidBounds.pMax - centroidBounds.pMin;

	int axis;
	if (diagonal.x > diagonal.y && diagonal.x > diagonal.z)
		axis = 0;
	else if (diagonal.y > diagonal.z)
		axis = 1;
	else
		axis = 2;

	if (centroidBounds.pMax[axis] == centroidBounds.pMin[axis]) {
		// �ʵ���ȷš�����ͬ��Ҷ���
		BBox3f pBounds;
		size_t index = orderedPrims.size();

		for (size_t i = start; i <= end; ++i) {
			orderedPrims.push_back(primitives[primitiveInfo[i].pIndex]);
			/*std::cout << std::format("before {} MakeLeaf [{}, {}, {}] [{}, {}, {}]\n", i,
				primitiveInfo[i].bounds.pMin.x, primitiveInfo[i].bounds.pMin.y, primitiveInfo[i].bounds.pMin.z,
				primitiveInfo[i].bounds.pMax.x, primitiveInfo[i].bounds.pMax.y, primitiveInfo[i].bounds.pMax.z);

			std::cout << std::format("{}\n", primitives[primitiveInfo[i].pIndex]->GetInfoString());*/

			pBounds = Union(pBounds, primitiveInfo[i].bounds);
		}

		tree[node_id].MakeLeaf(node_id, index, end - start + 1, pBounds);

		// push N
		BVHBuildTask task;
		task.type = 'N';
		task.node_id = node_id;
		task_q.push_back(task);

		return;// node_id;
	}

	// ����bin��Ϣ
	//std::vector<BVHBin> bins(binCount);
	BVHBin bins[12];

	int bin;

	for (size_t i = start; i <= end; ++i) {

		bin = binCount * (0.0001f + primitiveInfo[i].centroid[axis] - centroidBounds.pMin[axis]) /
			(centroidBounds.pMax[axis] - centroidBounds.pMin[axis]);

		if (bin > binCount - 1)
			bin = binCount - 1;

		//std::cout << std::format("i {} bin {} {} {} {} ", i, bin, primitiveInfo[i].centroid[axis], centroidBounds.pMin[axis], centroidBounds.pMax[axis]) << std::endl;

		bins[bin].count++;
		bins[bin].bounds.Union(primitiveInfo[i].bounds);
	}

	// ����bins����sah��
	// ���Ǽ�������������е�binΪ�ա�
	// ȫ��һ��bin���񲻿��ܣ�������һͷһβ��
	size_t countL = 0;
	BBox3f bboxL;
	float bestCost = MaxFloat;
	size_t bestBin;

	for (size_t i = 0; i < binCount - 1; ++i) {
		if (bins[i].count == 0) // �յ�����
			continue;

		// ��i�ұ��п�
		// ��ߵ�ֵ
		countL += bins[i].count;
		bboxL.Union(bins[i].bounds);

		// �ұߵ�ֵ
		size_t countR = 0;
		BBox3f bboxR;
		for (size_t j = i + 1; j < binCount; ++j) {
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

	// ����bestBin�ֳ����顣���������㡣
	auto pmid = std::partition(&primitiveInfo[start], &primitiveInfo[end] + 1,
		[&](const BVHPrimitiveInfo& i) {
			int bin = binCount * (0.0001f + i.centroid[axis] - centroidBounds.pMin[axis]) /
				(centroidBounds.pMax[axis] - centroidBounds.pMin[axis]);

			if (bin > binCount - 1)
				bin = binCount - 1;

			return bin <= bestBin;
		}
	);

	int mid = pmid - &primitiveInfo[0];

	//std::cout << "mid = " << mid << std::endl;


	//size_t l_id = Build(primitiveInfo, start, mid - 1, orderedPrims);
	//size_t r_id = Build(primitiveInfo, mid, end, orderedPrims);

	//std::cout << "MakeInterior " << node_id << " l = " << l_id << " r = " << r_id << " "<< ( & tree[l_id])->id << " " << (&tree[r_id])->id << std::endl;
	//tree[node_id].MakeInterior(node_id, &tree[l_id], &tree[r_id]);

	// push MBB
	BVHBuildTask task;
	task.type = 'M';
	task.node_id = node_id;
	task_q.push_back(task);

	task.type = 'B';
	task.start = start;
	task.end = mid - 1;
	task_q.push_back(task);

	task.type = 'B';
	task.start = mid;
	task.end = end;
	task_q.push_back(task);

	return;// node_id;
}

bool BVH::Intersect(const Ray& ray) const {
	if (tree.empty())
		return false;

	Vector3f invDir(1 / ray.d.x, 1 / ray.d.y, 1 / ray.d.z);
	int dirIsNeg[3] = { invDir.x < 0, invDir.y < 0, invDir.z < 0 };

	//std::vector<std::shared_ptr<BVHNode>> hopes;
	std::vector<const BVHNode*> hopes;
	hopes.reserve(tree.size() / 2);

	hopes.push_back(&tree[0]);

	float t;

	// ���ɺ���ȳ���������ȡ�

	while (true) {
		if (hopes.empty()) // ���п����þ�
			return false;

		auto hope = hopes.back();
		hopes.pop_back();

		// ���bbox�ཻ
		if (hope->bounds.Intersect(ray, invDir, dirIsNeg, &t)) {
			// �����Ҷ��
			if (hope->left == nullptr && hope->right == nullptr) {
				for (size_t i = 0; i < hope->count; ++i) {
					if (this->primitives[hope->index + i]->Intersect(ray)) {
						// primitiveȷʵ�ཻ
						return true;
					}
				}
			}
			else {
				// todo-�жϷ���������ʴ�ġ�
				hopes.push_back(hope->left);
				hopes.push_back(hope->right);
			}
		}
	}
}


bool BVH::Intersect(const Ray& ray, float* tHit, SurfaceInteraction* isect, int pool_id) {
	if (tree.empty())
		return false;
	//hopes_mutex.lock();
	//std::cout << "Intersect " << pool_id << " "<<std::this_thread::get_id() << std::endl;
	//hopes_mutex.unlock();
	Vector3f invDir(1 / ray.d.x, 1 / ray.d.y, 1 / ray.d.z);
	int dirIsNeg[3] = { invDir.x < 0, invDir.y < 0, invDir.z < 0 };

	float t;

	if (!tree[0].bounds.Intersect(ray, invDir, dirIsNeg, &t))
		return false;

	//std::vector<const BVHNode*>& hopes = hopes_pool[pool_id]; // ���߳���local��pool����ͦ��

	//std::vector<const BVHNode*> hopes;
	//hopes.push_back(&tree[0]);

	bool hit = false;
	float tMin = std::numeric_limits<float>::max();
	SurfaceInteraction tempIsect;

	//int pool_id = GetHopesPool();
	auto hopes = hopes_pool[pool_id];
	int qend = 0;
	hopes[qend] = &tree[0];

	// ���ɺ���ȳ�q��������ȡ���ȥ��һ������ϣ����bbox��
	while (true) {
		if (qend < 0) // ���п����þ�
			break;

		const BVHNode* hope = hopes[qend];
		qend--;

		// �����Ҷ��
		if (hope->left == nullptr && hope->right == nullptr) {
			// primitiveȷʵ�ཻ
			for (size_t i = 0; i < hope->count; ++i) {
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
			// todo �жϷ���

			// bbox�󽻵�t>=tMin�Ļ�û��Ҫ�ٲ顣
			if (hope->left->bounds.Intersect(ray, invDir, dirIsNeg, &t)) {
				if (t < tMin) {
					qend++;
					hopes[qend] = hope->left;
				}
			}

			if (hope->right->bounds.Intersect(ray, invDir, dirIsNeg, &t)) {
				if (t < tMin) {
					qend++;
					hopes[qend] = hope->right;
				}
			}
		}
	}

	//ReleaseHopesPool(pool_id);

	return hit;
}


void BVH::Print() const {
	std::cout << "BVH::Print()\n";
	PrintNode(0, -1);
}

void BVH::PrintNode(size_t node_id, size_t parent_id) const {
	auto node = &tree[node_id];
	std::cout << std::format("id {} pid {}, {}, {}, left {}, right {} [{}, {}, {}] [{}, {}, {}]\n", node->id, parent_id, node->index, node->count,
		size_t(node->left), size_t(node->right), 
		node->bounds.pMin.x, node->bounds.pMin.y, node->bounds.pMin.z,
		node->bounds.pMax.x, node->bounds.pMax.y, node->bounds.pMax.z);

	for (size_t i = 0; i < node->count; ++i) {
		std::cout << std::format("{}\n", primitives[node->index + i]->GetInfoString());
	}

	if (node->left != nullptr)
		PrintNode(node->left->id, node->id);

	if (node->right != nullptr)
		PrintNode(node->right->id, node->id);
}