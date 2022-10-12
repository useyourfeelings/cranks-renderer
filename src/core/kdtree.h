#ifndef CORE_KDTREE_H
#define CORE_KDTREE_H

#include <vector>
#include <queue>
#include <memory>
#include <string>
#include "geometry.h"


// k-d tree
// k = 3
// balanced

struct KDTNode {
	Point3f point;
	//int split; // 0-x 1-y 2-z
	//int index; // left = 2 * index + 1; right = left + 1
	int hasLeft;
	int hasRight;
};

struct NodeInfo
{
	float distance;
	int index;
	bool operator<(const NodeInfo& rhs) const {
		return distance < rhs.distance;
	}
};

struct KNNInfo {
	Point3f point; // to query
	int k;
	float resultMaxDistance = MaxFloat;
	std::priority_queue<NodeInfo> result;

	int pointsPruned; // rough count
};

class KDTree {
public:
	KDTree(const std::vector<Point3f> &points);
	void KNN(Point3f point, int k);
	void Test();

private:
	std::vector<KDTNode> nodes;

	// levels = int(log2(input_nodes_count + 1))
	// vector reserve = pow(2, levels) - 1
	int nodesCount;
	int levels;

	void Build(const std::vector<Point3f>& points, int test);
	void KNNQuery(KNNInfo& info, int currentIndex, int level);
	
	// for testing
	void KNN(Point3f point, int k, const std::vector<float> testResult);
};


#endif