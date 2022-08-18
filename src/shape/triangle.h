#ifndef CORE_TRIANGLE_H
#define CORE_TRIANGLE_H

#include "../core/shape.h"

// mesh����
class TriangleMesh {
public:
	// TriangleMesh Public Methods
	TriangleMesh(const Transform& ObjectToWorld, int nTriangles,
		const int* vertexIndices, int nVertices, 
		//const Point3f* P
        const float* P
		//const Vector3f* S,
		//const Normal3f* N,
		//const Point2f* uv,
		//const std::shared_ptr<Texture<float>>& alphaMask,
		//const std::shared_ptr<Texture<float>>& shadowAlphaMask,
		//const int* faceIndices
	);

	// TriangleMesh Data
	const int nTriangles; // ��������
	const int nVertices; // ����
	std::vector<int> vertexIndices; // ������ÿ�������index��ÿ������������index��˳���š�
	std::unique_ptr<Point3f[]> p;
	//std::unique_ptr<Normal3f[]> n;
	//std::unique_ptr<Vector3f[]> s;
	//std::unique_ptr<Point2f[]> uv;
	//std::shared_ptr<Texture<float>> alphaMask, shadowAlphaMask;
	//std::vector<int> faceIndices;
};

// ������
class Triangle : public Shape {
public:
    // Triangle Public Methods
    Triangle(const Transform& ObjectToWorld, const Transform& WorldToObject,
        //bool reverseOrientation, 
        const std::shared_ptr<TriangleMesh>& mesh,
        int triIndex)
        : Shape(ObjectToWorld, WorldToObject), mesh(mesh) {
        v = &mesh->vertexIndices[3 * triIndex];

        //triMeshBytes += sizeof(*this);
        //faceIndex = mesh->faceIndices.size() ? mesh->faceIndices[triNumber] : 0;
    }
    BBox3f ObjectBound() const;
    BBox3f WorldBound() const;

    bool Intersect(const Ray& ray, float* tHit, SurfaceInteraction* isect) const;
    bool Intersect(const Ray& ray) const;
    //float Area() const;

    //using Shape::Sample;  // Bring in the other Sample() overload.
    Interaction Sample(const Point2f& u, float* pdf) const;

    // Returns the solid angle subtended by the triangle w.r.t. the given
    // reference point p.
    //Float SolidAngle(const Point3f& p, int nSamples = 0) const;

    std::string GetInfoString() const;

private:

    std::shared_ptr<TriangleMesh> mesh;
    const int* v;
    //int faceIndex;
};

#endif