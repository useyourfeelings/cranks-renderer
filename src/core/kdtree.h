#ifndef CORE_KDTREE_H
#define CORE_KDTREE_H

#include <vector>
#include <queue>
#include <memory>
#include <string>
#include <random>
#include <cmath>
#include "geometry.h"


// k-d tree
// k = 3
// balanced

template <typename T>
struct KDTNode {
	T payload;
	Point3f point;
	//int index; // left = 2 * index + 1; right = left + 1
	int hasLeft;
	int hasRight;
};

struct NodeInfo
{
	float distance2;
	int index;
	bool operator<(const NodeInfo& rhs) const {
		return distance2 < rhs.distance2;
	}
};

struct KNNInfo {
	Point3f point; // to query
	int k;
	float resultMaxDistance2 = MaxFloat;
	std::priority_queue<NodeInfo> result;

	int pointsPruned; // rough count
};

template <typename T>
struct KNNResult {
	float resultMaxDistance2;
	std::vector<T> payloads;
};

struct BuildTask {
	int start;
	int end;
	int level; // from 0
	int kdtree_index; // where to put the median

	BuildTask(int start, int end, int level, int kdtree_index) :
		start(start),
		end(end),
		level(level),
		kdtree_index(kdtree_index) {
	}
};

template <typename T>
class KDTree {
public:
	KDTree() {}

	KDTree(const std::vector<T> & input_points) {
		Build(input_points, 0);
	}

	KNNResult<T> KNN(Point3f point, int k, float maxDistance2Limit = 0);
	void Test();

	void Build(const std::vector<T>& points, int test = 0);

	int Size() {
		return nodesCount;
	}

private:
	std::vector<KDTNode<T>> nodes;

	// levels = int(log2(input_nodes_count + 1))
	// vector reserve = pow(2, levels) - 1
	int nodesCount;
	int levels;
	
	void KNNQuery(KNNInfo& info, int currentIndex, int level);
	
	// for testing
	void KNN(Point3f point, int k, const std::vector<float> testResult);
};

template <typename T>
void KDTree<T>::Test() {
	std::cout << "KDTree::Test()" << std::endl;

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<> dis(0, 1);

	std::vector<T> input_points;

	int pointsCount = 10000;
	int testCount = 100;

	for (int i = 0; i < pointsCount; ++i) {
		float x = dis(gen) * 2000 - 1000;
		float y = dis(gen) * 2000 - 1000;
		float z = dis(gen) * 2000 - 1000;

		T t;
		t.pos = Point3f(x, y, z);
		input_points.push_back(t);
	}

	Build(input_points, testCount);
}

template <typename T>
void KDTree<T>::Build(const std::vector<T>& input_points, int test) {
	std::cout << "KDTree::Build()" << std::endl;
	
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<> dis(0, 1);

	std::vector<T> points = input_points;
	nodesCount = points.size();

	levels = int(std::ceil(std::log2(nodesCount + 1)));
	nodes.resize(size_t(std::pow(2, levels)) - 1);

	std::cout << "nodesCount = " << nodesCount << " levels = " << levels << " nodes.size() = " << nodes.size() << std::endl;

	std::vector<BuildTask> build_tasks; // start end level
	build_tasks.push_back(BuildTask(0, points.size() - 1, 0, 0));

	while (!build_tasks.empty()) { // pick median in [start, end], set kdtree_index.
		BuildTask build_task = build_tasks.back();
		build_tasks.pop_back();

		//std::cout <<"-------- build_task:"<< build_task.start << " " << build_task.end << " " << build_task.level << " " << build_task.kdtree_index << std::endl;

		int axis = build_task.level % 3;

		int index_to_find = (build_task.start + build_task.end) / 2; // find median
		//std::cout << "index_to_find = " << index_to_find << std::endl;

		int start = build_task.start;
		int end = build_task.end;
		for (;;) {
			//std::cout << "find median " << start << " " << end << std::endl;
			/*if (start == end) {
				std::cout << "set " << build_task.kdtree_index << std::endl;
				this->nodes[build_task.kdtree_index] = KDTNode({ points[index_to_find], axis, false, false });
				break;
			}*/

			int random_index = start + int(float(dis(gen)) * (end - start)); // choose random pivot
			int pivot = points[random_index].pos[axis];

			//std::cout << "random_index = " << random_index << std::endl;

			// swap to end
			std::swap(points[end], points[random_index]);

			// partition
			int i = start - 1;
			for (int j = start; j < end; ++j) {
				if (points[j].pos[axis] <= pivot) {
					++i;

					std::swap(points[i], points[j]);
				}
			}

			// swap end back
			++i;
			std::swap(points[end], points[i]);

			// partition over

			// 都相等的情况 points[start][axis] == points[end][axis]
			// 如果中间存在一批相等。
			// 直接取中间一个，会造成右边存在相等的值。暂时这样做看看效果。
			// 如果不取中间的，树就不平衡。

			//std::cout << "i = " << i << std::endl;

			if ((i == index_to_find) ||
				(points[start].pos[axis] == points[end].pos[axis])) {  // found

				KDTNode<T> newNode;
				newNode.payload = points[index_to_find];
				newNode.point = points[index_to_find].pos;
				//newNode.split = axis;
				newNode.hasLeft = false;
				newNode.hasRight = false;

				if (build_task.start < index_to_find) {
					//std::cout << "build_tasks push << " << build_task.start << " " << index_to_find - 1 << std::endl;
					build_tasks.push_back(BuildTask(build_task.start, index_to_find - 1, build_task.level + 1, build_task.kdtree_index * 2 + 1));
					newNode.hasLeft = true;
				}

				if (index_to_find < build_task.end) {
					//std::cout << "build_tasks push << " << index_to_find + 1 << " " << build_task.end << std::endl;
					build_tasks.push_back(BuildTask(index_to_find + 1, build_task.end, build_task.level + 1, build_task.kdtree_index * 2 + 2));
					newNode.hasRight = true;
				}

				// set nodes
				//std::cout << "set " << build_task.kdtree_index << std::endl;
				this->nodes[build_task.kdtree_index] = newNode;
				break;
			}
			else {
				if (index_to_find > i) // i too small
					start = i + 1;
				else
					end = i - 1;
			}
		}
	}

	std::cout << "KDTree::KDTree() over" << std::endl;

	//for (auto n : nodes) {
		//std::cout<< n.point[0] << " " << n.point[1] << " "<< n.point[2] << std::endl;
	//}

	// test
	if (test) {
		float minX = MaxFloat;
		float minY = MaxFloat;
		float minZ = MaxFloat;
		float maxX = MinFloat;
		float maxY = MinFloat;
		float maxZ = MinFloat;

		for (auto p : points) {
			if (p.pos.x < minX) minX = p.pos.x;
			if (p.pos.x > maxX) maxX = p.pos.x;
			if (p.pos.y < minY) minY = p.pos.y;
			if (p.pos.y > maxY) maxY = p.pos.y;
			if (p.pos.z < minZ) minZ = p.pos.z;
			if (p.pos.z > maxZ) maxZ = p.pos.z;
		}

		for (int i = 0; i < test; ++i) {
			auto rand = dis(gen);
			auto k = int(1 + dis(gen) * 100); // int(1 + dis(e) * input_points.size()) / 10;

			Point3f point(
				minX - (maxX - minX) / 2 + 2 * rand * (maxX - minX),
				minY - (maxY - minY) / 2 + 2 * rand * (maxY - minY),
				minZ - (maxZ - minZ) / 2 + 2 * rand * (maxZ - minZ));

			std::vector<float> testResult;

			for (auto ip : input_points) {
				testResult.push_back(DistanceSquared(point, ip.pos));
			}

			std::sort(testResult.begin(), testResult.end());

			KNN(point, k, testResult);
		}
	}
}


template <typename T>
void KDTree<T>::KNN(Point3f point, int k, const std::vector<float> testResult) {
	std::cout << "test " << k << "NN " << point.x << " " << point.y << " " << point.z << std::endl;
	KNNInfo info;
	info.point = point;
	info.k = k;
	info.pointsPruned = 0;

	KNNQuery(info, 0, 0);

	std::cout << "KNN over\nanswer:\n";

	for (auto a : testResult) {
		//std::cout << a <<std::endl;
	}

	//std::cout << "---\n";

	int index = info.result.size() - 1;
	while (!info.result.empty()) {
		//std::cout << info.result.top().distance << " " << info.result.top().index << std::endl;

		if (testResult[index] != info.result.top().distance2) {
			std::cout << "KNN test failed\n";
			throw("KNN test failed");
			return;
		}

		--index;

		info.result.pop();
	}

	std::cout << "KNN test ok\npoints pruned = " << info.pointsPruned << std::endl;
}

template <typename T>
KNNResult<T> KDTree<T>::KNN(Point3f point, int k, float maxDistance2Limit) {
	//std::cout << "KNN\n";
	KNNInfo info;
	info.point = point;
	info.k = k;
	info.pointsPruned = 0;

	// todo 把maxDistance2Limit做到query里。提升效率。
	if (maxDistance2Limit != 0)
		info.resultMaxDistance2 = maxDistance2Limit;

	KNNQuery(info, 0, 0);

	KNNResult<T> result;

	//result.resultMaxDistance2 = info.resultMaxDistance2;

	if (maxDistance2Limit == 0)
		result.resultMaxDistance2 = info.resultMaxDistance2;
	else
		result.resultMaxDistance2 = -1;

	//std::cout << "KNN over\n";
	while (!info.result.empty()) {
		if (maxDistance2Limit != 0) { // with limit
			if (info.result.top().distance2 > maxDistance2Limit) {
				info.result.pop();
				continue;
			}

			if(result.resultMaxDistance2 == -1)
				result.resultMaxDistance2 = info.result.top().distance2;
		}

		result.payloads.push_back(nodes[info.result.top().index].payload);

		//std::cout << info.result.top().distance2 << " " << info.result.top().index << std::endl;
		info.result.pop();
	}

	return result;
}

template <typename T>
void KDTree<T>::KNNQuery(KNNInfo& info, int currentIndex, int level) {

	float distance2 = DistanceSquared(info.point, nodes[currentIndex].point);

	// not full or better distance
	if (info.result.size() < info.k || distance2 < info.resultMaxDistance2) {
		info.result.push(NodeInfo{ distance2, currentIndex });

		if (info.result.size() > info.k) // at most k
			info.result.pop();

		info.resultMaxDistance2 = info.result.top().distance2;
	}

	if (nodes[currentIndex].hasLeft) {
		// 如果候选已满，且resultMaxDistance比此分支还近，那么剪枝。
		if (info.result.size() >= info.k &&
			(info.point[level % 3] - nodes[currentIndex].point[level % 3]) > info.resultMaxDistance2) {
			//std::cout << " !!!!!!!!!!!!!!!!!!!!!! prune left level " << level + 1 << " total level = "<< levels <<std::endl;
			//info.pointsPruned += int(std::pow(2, levels - level - 2)); // rough count
		}
		else {
			KNNQuery(info, currentIndex * 2 + 1, level + 1);
		}
	}

	if (nodes[currentIndex].hasRight) {
		// 如果候选已满，且resultMaxDistance比此分支还近，那么剪枝。
		if (info.result.size() >= info.k &&
			(nodes[currentIndex].point[level % 3] - info.point[level % 3]) > info.resultMaxDistance2) {
			//std::cout << " !!!!!!!!!!!!!!!!!!!!!! prune right level " << level + 1 << " total level = " << levels << std::endl;
			//info.pointsPruned += int(std::pow(2, levels - level - 2)); // rough count
		}
		else {
			KNNQuery(info, currentIndex * 2 + 2, level + 1);
		}
	}
}

#endif