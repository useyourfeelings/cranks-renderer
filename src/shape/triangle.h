#ifndef CORE_TRIANGLE_H
#define CORE_TRIANGLE_H

#include "../core/shape.h"
#include "../core/object.h"
#include "../core/scene.h"

// mesh数据
class TriangleMesh: public Object {
public:
    // TriangleMesh Public Methods
    TriangleMesh(const json& config, const Transform& ObjectToWorld, int nTriangles,
        const int* vertexIndices, int nVertices,
        //const Point3f* P
        const float* P,
        //const Vector3f* S,
        const float* N,
        //const Point2f* uv,
        //const std::shared_ptr<Texture<float>>& alphaMask,
        //const std::shared_ptr<Texture<float>>& shadowAlphaMask,
        //const int* faceIndices
        //int material_id
        std::shared_ptr<Material> material,
        std::shared_ptr<Scene> scene,
        std::shared_ptr<Medium> medium
    );

    // TriangleMesh Data
    const int nTriangles; // 三角形数
    const int nVertices; // 点数
    std::vector<int> vertexIndices; // 三角形每个顶点的index。每个三角形三个index，顺序排。
    std::unique_ptr<Point3f[]> p;
    std::unique_ptr<Vector3f[]> n;
    //std::unique_ptr<Vector3f[]> s;
    //std::unique_ptr<Point2f[]> uv;
    //std::shared_ptr<Texture<float>> alphaMask, shadowAlphaMask;
    //std::vector<int> faceIndices;

    std::shared_ptr<Material> GetMaterial();

private:
    int material_id;
    std::weak_ptr<Material> material;
    std::weak_ptr<Scene> scene;
    std::weak_ptr<Medium> medium;
};

// 三角形
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
    const int* v; // 指向mesh的该三角index处
    //int faceIndex;
};

#endif